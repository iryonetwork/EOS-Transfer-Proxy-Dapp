#include <eosiolib/action.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>

#include <string>
#include <utility>

using namespace eosio;


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


class transfer_proxy : private contract
{
public:
    transfer_proxy(account_name self) : 
        contract(self),
        owner_(self, self)
    {}

    // @abi action
    void setowner(account_name owner)
    {
        require_auth(_self);
        owner_.set(owner, _self);
    }

    void on_transfer(account_name code, transfer_args t)
    {
        auto owner = owner_.get();
        eosio_assert(t.from == owner || t.from == _self || t.from == N(eosio.ram), "invalid transfer");
        if(t.from == owner && t.to == _self)
        {
            std::size_t recip_end_pos = 0;
            account_name recipient = get_recipient_from_memo(t.memo, recip_end_pos);
            strip_receipient_from_memo(t.memo, recip_end_pos);
            
            eosio_assert(recipient != _self && recipient != owner, "invalid recipient");
            dispatch_inline(code,  N(transfer), {{_self, N(active)}}, 
                std::make_tuple(_self, recipient, t.amount, std::move(t.memo))
            );
        }
    }

private:
    singleton<N(owner), account_name> owner_; 
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

EOSIO_ABI(transfer_proxy, (setowner))