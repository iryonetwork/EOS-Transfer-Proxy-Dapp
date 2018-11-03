#include <eosiolib/action.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/singleton.hpp>

#include <string>
#include <utility>

using namespace eosio;
static constexpr auto k_transfer = "transfer"_n;

class proxy_test : private contract
{
public:
    proxy_test(name self, name code, datastream<const char*> ds ) : 
        contract(self, code, ds),
        proxy_(self, self.value)
    {}

    [[eosio::action]]
    void setproxy(name proxy)
    {
        require_auth(_self);
        proxy_.set(proxy, _self);
    }

    [[eosio::action]]
    void open(name owner, name token_contract, asset token)
    {
        accounts acnt(_self, owner.value);
        auto tkn = acnt.find(token.symbol.code().raw());
        eosio_assert(tkn == acnt.end() && 
            token_contract != tkn->balance.contract, "account already exists");

        acnt.emplace(owner, [&]( auto& a) {
            a.balance = extended_asset(0,
                extended_symbol{token.symbol, name{token_contract}}
            );
        });
    }

    [[eosio::action]]
    void close(name owner, name token_contract, asset token)
    {
        accounts acnt(_self, owner.value);
        auto tkn = acnt.get(token.symbol.code().raw(), "account doesn't exists");
        eosio_assert(tkn.balance.quantity.amount == 0, "balance must be withdrawn first");
        acnt.erase(tkn);
    }

    [[eosio::action]]
    void withdraw(name account, asset quantity, std::string memo)
    {
        accounts acnt(_self, account.value);
        const auto& from = acnt.get(quantity.symbol.code().raw(), "account not found");
        eosio_assert(from.balance.quantity.amount >= quantity.amount, "overdrawn balance");

        transfer_via_proxy(account, extended_asset(quantity, from.balance.contract), std::move(memo));
        acnt.modify(from, account, [&](auto& a) {
            a.balance.quantity -= quantity;
        });
    }

    void on_transfer(name from, name to, asset amount, std::string memo)
    {
        if(to == _self)
        {
            accounts acnt(_self, from.value);
            auto tkn = acnt.find(amount.symbol.code().raw());
            if(tkn == acnt.end())
            {
                acnt.emplace(_self, [&]( auto& a) {
                    a.balance = extended_asset(amount, _code);
                });
            }
            else 
            {
                eosio_assert(tkn->balance.contract == _code, "invalid token transfer");
                acnt.modify(tkn, same_payer, [&](auto& a) {
                    a.balance.quantity += amount;
                });
            }
        }
    }

private:
    void transfer_via_proxy(name to, extended_asset amount, std::string memo)
    {
        auto proxy = proxy_.get();
        dispatch_inline(proxy,  k_transfer, {{ _self, "active"_n }}, 
            std::make_tuple(_self, to, std::move(amount), std::move(memo))
        );
    }


private:
    singleton<"proxy"_n, name > proxy_;

    struct [[eosio::table]] account
    {
        extended_asset    balance;
        uint64_t primary_key() const { return balance.quantity.symbol.code().raw(); }
    };

    typedef eosio::multi_index<
        "accounts"_n, account
    > accounts;

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

EOSIO_DISPATCH(proxy_test, (setproxy)(open)(close)(withdraw))