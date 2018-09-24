#include <eosiolib/action.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/singleton.hpp>

#include <string>
#include <utility>

using namespace eosio;


struct transfer_args
{
    account_name from;
    account_name to;
    asset amount;
    std::string memo;
};


class proxy_test : private contract
{
public:
    proxy_test(account_name self) : 
        contract(self),
        proxy_(self,self)
    {}

    [[eosio::action]]
    void setproxy(account_name proxy)
    {
        require_auth(_self);
        proxy_.set(proxy, _self);
    }

    [[eosio::action]]
    void open(account_name owner, account_name token_contract, asset token)
    {
        accounts acnt(_self, owner);
        auto tkn = acnt.find(token.symbol.name());
        eosio_assert(tkn == acnt.end() && 
            token_contract != tkn->balance.contract, "account already exists");

        acnt.emplace(owner, [&]( auto& a) {
            a.balance = extended_asset(0,
                extended_symbol{token.symbol, name{token_contract}}
            );
        });
    }

    [[eosio::action]]
    void close(account_name owner, account_name token_contract, asset token)
    {
        accounts acnt(_self, owner);
        auto tkn = acnt.get(token.symbol.name(), "account doesn't exists");
        eosio_assert(tkn.balance.quantity.amount == 0, "balance must be withdrawn first");
        acnt.erase(tkn);
    }

    [[eosio::action]]
    void withdraw(account_name account, asset quantity, std::string memo)
    {
        accounts acnt(_self, account);
        const auto& from = acnt.get(quantity.symbol.name(), "account not found");
        eosio_assert(from.balance.quantity.amount >= quantity.amount, "overdrawn balance");

        transfer_via_proxy(account, extended_asset(quantity, from.balance.contract), std::move(memo));
        acnt.modify(from, account, [&](auto& a) {
            a.balance.quantity -= quantity;
        });
    }

    void on_transfer(account_name code, transfer_args t)
    {
        if(t.to == _self)
        {
            accounts acnt(_self, t.from);
            auto tkn = acnt.find(t.amount.symbol.name());
            if(tkn == acnt.end())
            {
                acnt.emplace(_self, [&]( auto& a) {
                    a.balance = extended_asset(t.amount, name{code});
                });
            }
            else 
            {
                eosio_assert(tkn->balance.contract == code, "invalid token transfer");
                acnt.modify(tkn, 0, [&](auto& a) {
                    a.balance.quantity.amount += t.amount.amount;
                });
            }
        }
    }

private:
    void transfer_via_proxy(account_name to, extended_asset amount, std::string memo)
    {
        auto proxy = proxy_.get();
        dispatch_inline(proxy,  "transfer"_n, {{_self, "active"_n}}, 
            std::make_tuple(_self, to, std::move(amount), std::move(memo))
        );
    }


private:
    singleton<N(proxy), account_name > proxy_;

    struct [[eosio::table]] account
    {
        extended_asset    balance;
        uint64_t primary_key() const { return balance.quantity.symbol.name(); }
    };

    typedef eosio::multi_index<
        N(accounts), account
    > accounts;

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

EOSIO_ABI(proxy_test, (setproxy)(open)(close)(withdraw))