#pragma once
#include <boost/algorithm/string/trim.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/multi_index.hpp>
#include <string>

namespace proxy {
    std::string name_to_string(uint64_t acct_int) 
    {
        static constexpr const char* charmap = ".12345abcdefghijklmnopqrstuvwxyz";
        std::string str(13,'.');
        uint64_t tmp = acct_int;

        for( uint32_t i = 0; i <= 12; ++i ) 
        {
            char c = charmap[tmp & (i == 0 ? 0x0f : 0x1f)];
            str[12-i] = c;
            tmp >>= (i == 0 ? 4 : 5);
        }

        boost::algorithm::trim_right_if( str, []( char c ){ return c == '.'; } );
        return str;
    }

    std::string gen_proxy_memo(account_name recipient, const std::string& memo)
    {
        return name_to_string(recipient) + " " + memo;
    }

    eosio::extended_asset get_transfer_fee(const eosio::extended_asset& value)
    {
        auto fee = value;
        fee.amount = (fee.amount + 999LL) /1000LL; // 0.1% rounded up
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