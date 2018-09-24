#pragma once
#include <boost/algorithm/string/trim.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/types.hpp>
#include <string>

namespace proxy {

    std::string gen_proxy_memo(account_name recipient, const std::string& memo)
    {
        return eosio::name{recipient}.to_string() + " " + memo;
    }

    eosio::extended_asset get_transfer_fee(const eosio::extended_asset& value)
    {
        auto fee = value;
        fee.quantity.amount = (fee.quantity.amount + 999LL) /1000LL; // 0.1% rounded up
        return fee;
    }

    struct token 
    {
        token(account_name tkn) : _self(tkn) {}
        struct account 
        {
            eosio::asset    balance;
            uint64_t primary_key()const { return balance.symbol.name(); }
        };
        typedef eosio::multi_index<N(accounts), account> accounts;

        bool has_balance(account_name owner,  eosio::symbol_type sym) const
        {
            accounts acnt(_self, owner);
            return acnt.find(sym.name()) != acnt.end();
        }

    private:
        account_name _self;
    };
}