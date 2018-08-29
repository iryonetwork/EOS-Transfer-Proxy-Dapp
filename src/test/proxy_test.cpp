#include <eosiolib/action.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/singleton.hpp>

#include <string>
#include <utility>

#include "../utils.hpp"

using namespace eosio;
using namespace proxy;


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

    // @abi action
    void setproxy(account_name proxy)
    {
        require_auth(_self);
        proxy_.set(proxy, _self);
    }

    // @abi action
    void withdraw(account_name account, asset quantity, std::string memo)
    {
        accounts acnt(_self, account);
        const auto& from = acnt.get(quantity.symbol.name(), "no balance object found");
        eosio_assert(from.balance.amount >= quantity.amount, "overdrawn balance");

        transfer_via_proxy(account, extended_asset(quantity, from.balance.contract), memo);

        if(from.balance.amount == quantity.amount) {
            acnt.erase(from);
        } 
        else {
            acnt.modify(from, account, [&](auto& a) {
                a.balance -= quantity;
            });
        }
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
                    a.balance = extended_asset(t.amount, code);
                });
            }
            else 
            {
                eosio_assert(tkn->balance.contract == code, "invalid token transfer");
                acnt.modify(tkn, 0, [&](auto& a) {
                    a.balance.amount += t.amount.amount;
                });
            }
        }
    }

private:
    void transfer_via_proxy(account_name to, extended_asset amount, const std::string& memo)
    {
        auto proxy = proxy_.get();
        dispatch_inline(amount.contract,  N(transfer), {{_self, N(active)}}, 
            std::make_tuple(_self, proxy, static_cast<asset&>(amount), gen_proxy_memo(to, memo))
        );
    }


private:
    singleton<N(proxy), account_name> proxy_;

    // @abi table accounts i64
    struct account
    {
        extended_asset    balance;
        uint64_t primary_key() const { return balance.symbol.name(); }
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

EOSIO_ABI(proxy_test, (setproxy)(withdraw))