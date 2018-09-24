#include <eosiolib/action.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>

#include <string>
#include <utility>

#include "utils.hpp"

using namespace eosio;
using namespace proxy;

static constexpr uint32_t tt_ram_bytes = 245; // new token transfer ram cost
static constexpr auto k_active    = "active"_n;
static constexpr auto k_eosio     = "eosio"_n;
static constexpr auto k_eosio_ram = "eosio.ram"_n;
static constexpr auto k_transfer  = "transfer"_n;

account_name get_recipient_from_memo(const std::string& memo, std::size_t& end_pos)
{
    end_pos = memo.find(" ");
    if(end_pos != memo.npos) end_pos++;
    return string_to_name(memo.substr(0, end_pos).c_str());
}

void strip_receipient_from_memo(std::string& memo, std::size_t end_pos)
{
    memo = memo.substr(end_pos);
}

struct transfer_args
{
    account_name from;
    account_name to;
    asset amount;
    std::string memo;
};

void transfer_token(account_name from, account_name to, const extended_asset& quantity, std::string memo)
{
    dispatch_inline(quantity.contract, k_transfer, {{ from, k_active }}, 
        std::make_tuple(from, to, quantity.quantity, std::move(memo))
    );
}


class transfer_proxy : private contract
{
    struct [[eosio::table]] account_t
    {
        account_name owner;
        bool free_transfer;
        uint64_t primary_key() const { return owner; }
    };
    multi_index<N(accounts), account_t> accounts;
    singleton<N(feerecipient), account_name> fee_recipient;

public:
    transfer_proxy(account_name self) : 
        contract(self),
        accounts(self, self),
        fee_recipient(self, self)
    {}

    [[eosio::action]]
    void signup(account_name owner)
    {
        require_auth(owner);
        auto it = accounts.find(owner);
        eosio_assert(it == accounts.end(), "account has already signed up");
        accounts.emplace(owner, [&](auto& a){
            a.owner = owner;
            a.free_transfer = false;
        });
    }

    [[eosio::action]]
    void unregister(account_name owner)
    {
        require_auth(owner);
        auto acnt = accounts.find(owner);
        accounts.erase(acnt);
    }

    [[eosio::action]]
    void transfer(account_name from, account_name to, extended_asset quantity, std::string memo)
    {
        require_auth(from);
        transfer_token(from, _self, quantity, gen_proxy_memo(to, memo));
    }

//private_api:
    [[eosio::action]]
    void setfreetxfr(account_name owner, bool free_transfer)
    {
        require_auth(_self);
        auto it = accounts.find(owner);
        accounts.modify(it, 0, [&](auto& a){
            a.free_transfer = free_transfer;
        });
    }

    [[eosio::action]]
    void setfeerecip(account_name recipient)
    {
        require_auth(_self);
        eosio_assert(is_account(recipient), "invalid account");
        fee_recipient.set(recipient, _self);
    }

    void on_transfer(account_name code, transfer_args t)
    {
        if(t.from != k_eosio_ram && t.to == _self)
        {
            auto acnt = accounts.get(t.from, "account doesn't exists");
            std::size_t recip_end_pos = 0;
            account_name recipient = get_recipient_from_memo(t.memo, recip_end_pos);
            eosio_assert(recipient != _self && recipient != acnt.owner, "invalid recipient");

            strip_receipient_from_memo(t.memo, recip_end_pos);
            make_transfer(acnt, recipient, extended_asset(t.amount, name{code}), std::move(t.memo));
        }
    }

private:
    void buy_ram_bytes(account_name buyer, uint32_t bytes) const
    {
        static constexpr auto k_buyrambytes = "buyrambytes"_n;
        dispatch_inline(k_eosio,  k_buyrambytes, {{ buyer, k_active}}, 
            std::make_tuple(buyer, _self, bytes)
        );
    }

    void make_transfer(const account_t& acnt, account_name recipient, extended_asset quantity, std::string memo)
    {
        if(!acnt.free_transfer) 
        {
            auto fee = get_transfer_fee(quantity);
            quantity -= fee;
            transfer_token_from_self(acnt.owner, fee_recipient.get(), fee, "Transfer fee");
        }
        transfer_token_from_self(acnt.owner, recipient, quantity, std::move(memo));
    }

    void transfer_token_from_self(account_name ram_payer, account_name recipient, const extended_asset& quantity, std::string memo)
    {
        if(quantity.quantity.amount > 0)
        {
            if(!token(quantity.contract).has_balance(recipient, quantity.quantity.symbol)) {
                buy_ram_bytes(ram_payer, tt_ram_bytes);
            }
            transfer_token(_self, recipient, quantity, std::move(memo));
        }
    }
};


#undef EOSIO_ABI
#define EOSIO_ABI(TYPE, MEMBERS) \
extern "C" {  \
    void apply( uint64_t receiver, uint64_t sender, uint64_t action ) { \
        auto self = receiver; \
        TYPE thiscontract( self ); \
        if(sender == self) { \
            switch( action ) { \
                EOSIO_API( TYPE, MEMBERS ) \
            } \
        } \
        else if(action == N(transfer)) { \
            thiscontract.on_transfer(sender, unpack_action_data<transfer_args>()); \
        } \
    } \
}

EOSIO_ABI(transfer_proxy, (signup)(unregister)(transfer)(setfreetxfr)(setfeerecip))