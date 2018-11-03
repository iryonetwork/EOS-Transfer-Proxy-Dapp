#pragma once
#include <eosiolib/asset.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/name.hpp>
#include <string>

namespace proxy {

    std::string gen_proxy_memo(eosio::name recipient, const std::string& memo)
    {
        return recipient.to_string() + " " + memo;
    }

    eosio::extended_asset get_transfer_fee(const eosio::extended_asset& value)
    {
        auto fee = value;
        fee.quantity.amount = (fee.quantity.amount + 999LL) /1000LL; // 0.1% rounded up
        return fee;
    }

    struct token 
    {
        token(eosio::name tkn) : _self(tkn) {}
        struct account 
        {
            eosio::asset    balance;
            uint64_t primary_key()const { return balance.symbol.code().raw(); }
        };
        using accounts = eosio::multi_index<"accounts"_n, account>;

        bool has_balance(eosio::name owner, eosio::symbol sym) const
        {
            accounts acnt(_self, owner.value);
            return acnt.find(sym.code().raw()) != acnt.end();
        }

    private:
        eosio::name _self;
    };
}