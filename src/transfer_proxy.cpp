#include <eosiolib/action.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>

#include <string>
#include <string_view>
#include <utility>

#include "utils.hpp"

using namespace eosio;
using namespace proxy;

static constexpr uint32_t tt_ram_bytes = 245; // new token transfer ram cost
static constexpr auto k_active    = "active"_n;
static constexpr auto k_eosio     = "eosio"_n;
static constexpr auto k_eosio_ram = "eosio.ram"_n;
static constexpr auto k_transfer  = "transfer"_n;
#include <stdlib.h>
name get_recipient_from_memo(const std::string& memo, std::size_t& end_pos)
{
    end_pos = memo.find(" ");
    name recipient(std::string_view(memo).substr(0, end_pos));
    if(end_pos != memo.npos) end_pos++;
    return recipient;
}

void strip_receipient_from_memo(std::string& memo, std::size_t end_pos)
{
    memo = memo.substr(end_pos);
}

void transfer_token(name from, name to, const extended_asset& quantity, std::string memo)
{
    dispatch_inline(quantity.contract, k_transfer, {{ from, k_active }}, 
        std::make_tuple(from, to, quantity.quantity, std::move(memo))
    );
}


class transfer_proxy : private contract
{
    struct [[eosio::table]] account_t
    {
        name owner;
        bool free_transfer;
        uint64_t primary_key() const { return owner.value; }
    };
    multi_index<"accounts"_n, account_t> accounts;
    singleton<"feerecipient"_n, name> fee_recipient;

public:
    transfer_proxy(name self, name code, datastream<const char*> ds) : 
        contract(self, code,ds),
        accounts(self, self.value),
        fee_recipient(self, self.value)
    {}

    [[eosio::action]]
    void signup(name owner)
    {
        require_auth(owner);
        auto it = accounts.find(owner.value);
        eosio_assert(it == accounts.end(), "account has already signed up");
        accounts.emplace(owner, [&](auto& a){
            a.owner = owner;
            a.free_transfer = false;
        });
    }

    [[eosio::action]]
    void unregister(name owner)
    {
        require_auth(owner);
        auto acnt = accounts.find(owner.value);
        accounts.erase(acnt);
    }

    [[eosio::action]]
    void transfer(name from, name to, extended_asset quantity, std::string memo)
    {
        require_auth(from);
        transfer_token(from, _self, quantity, gen_proxy_memo(to, memo));
    }

//private_api:
    [[eosio::action]]
    void setfreetxfr(name owner, bool free_transfer)
    {
        require_auth(_self);
        auto it = accounts.find(owner.value);
        accounts.modify(it, same_payer, [&](auto& a){
            a.free_transfer = free_transfer;
        });
    }

    [[eosio::action]]
    void setfeerecip(name recipient)
    {
        require_auth(_self);
        eosio_assert(is_account(recipient), "invalid account");
        fee_recipient.set(recipient, _self);
    }

    void on_transfer(name from, name to, asset amount, std::string memo)
    {
        if(from != k_eosio_ram && to == _self)
        {
            auto acnt = accounts.get(from.value, "account doesn't exists");
            std::size_t recip_end_pos = 0;
            name recipient = get_recipient_from_memo(memo, recip_end_pos);
            eosio_assert(recipient != _self && recipient != acnt.owner, "invalid recipient");

            strip_receipient_from_memo(memo, recip_end_pos);
            make_transfer(acnt, recipient, extended_asset(amount, _code), std::move(memo));
        }
    }

private:
    void buy_ram_bytes(name buyer, uint32_t bytes) const
    {
        static constexpr auto k_buyrambytes = "buyrambytes"_n;
        dispatch_inline(k_eosio,  k_buyrambytes, {{ buyer, k_active }}, 
            std::make_tuple(buyer, _self, bytes)
        );
    }

    void buy_transfer_ram(name ram_payer, name recipient, const extended_asset& quantity, name fee_recipient, const extended_asset& fee)
    {
        uint32_t bytes = 0;
        if(fee.quantity.amount > 0)
        {
            if(!token(fee.contract).has_balance(fee_recipient, fee.quantity.symbol)) {
                bytes += tt_ram_bytes;
            }
        }

        if(quantity.quantity.amount > 0)
        {
            if(!token(quantity.contract).has_balance(recipient, quantity.quantity.symbol)) {
                bytes += tt_ram_bytes;
            }
        }

        if(bytes > 0) {
            buy_ram_bytes(ram_payer, bytes);
        }
    }

    void make_transfer(const account_t& acnt, name recipient, extended_asset quantity, std::string memo)
    {
        extended_asset fee;
        if(!acnt.free_transfer) 
        {
            fee = get_transfer_fee(quantity);
            quantity -= fee;
        }

        const auto fee_fecipient = fee_recipient.get();
        buy_transfer_ram(acnt.owner, recipient, quantity, fee_fecipient, fee);
        transfer_token_from_self(fee_fecipient, fee, "Transfer fee");
        transfer_token_from_self(recipient, quantity, std::move(memo));
    }

    void transfer_token_from_self(name recipient, const extended_asset& quantity, std::string memo)
    {
        if(quantity.quantity.amount > 0) {
            transfer_token(_self, recipient, quantity, std::move(memo));
        }
    }
};


#undef EOSIO_DISPATCH
#define EOSIO_DISPATCH(TYPE, MEMBERS) \
extern "C" { \
    void apply(uint64_t receiver, uint64_t code, uint64_t action) { \
        if(code == receiver) { \
            switch(action) { \
                EOSIO_DISPATCH_HELPER(TYPE, MEMBERS) \
            } \
        } \
        else if(action == k_transfer.value) { \
            execute_action(name(receiver), name(code), &TYPE::on_transfer); \
        } \
    } \
}

EOSIO_DISPATCH(transfer_proxy, (signup)(unregister)(transfer)(setfreetxfr)(setfeerecip))