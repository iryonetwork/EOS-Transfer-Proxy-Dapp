# EOS Transfer Proxy dapp

#### What does it do
Put this contract onto an account with no available RAM and then use it to proxy token transfers from your dapp to users that might have code installed.

#### The problem
A malicious user can install code on their account which will allow them to insert rows in the name of another account sending them tokens. This lets them lock up RAM by inserting large amounts of garbage into rows when dapps/users send them tokens.

#### The solution
By sending tokens to a proxy account with no available RAM, and with a memo where the first word of the memo is the account you eventually want to send the tokens to, the only account they can assume database row permissions for is the proxy, which has no RAM.

#### Token types
This contract accepts all token types that conform to the basic eosio.token contract. The only method that has to have an identical argument signature is the transfer method.

### How to setup your account with proxy
1. You will first have to signup your account with proxy by calling `signup` action with your account name.

    `cleos push action <proxy_contract> signup '["<your_account>"]' -p <your_account>`
  
2. In case your contract transfers other tokens than EOS you have to add `eosio.code` permission to contract's account and authorise proxy contract to have `eosio.code` permission of contract's active authority. This will allow the proxy contract to buy ram in case needed for the token transfer. Make sure you have enough EOS on your account, otherwise the buy ram action (and whole transfer) will fail.

    ```
    cleos set account permission <your_account> active \
    '{"threshold": 1,"keys": [{"key": "<your_account_public_key>","weight": 1}],"accounts": [{"permission":{"actor":"<proxy_contract_account>","permission":"eosio.code"},"weight":1},{"permission":{"actor":"<your_account>","permission":"eosio.code"},"weight":1}]}' owner
    ```
See also [add_transfer_proxy.sh](scripts/add_transfer_proxy.sh).  
### How to make transfer via proxy
After you have set up your account with the proxy contract you are ready to make token transfer via proxy.
There are two ways to make transfer:

1. call proxy action `transfer`
2. transfer tokens to proxy contract

#### Proxy transfer via transfer action
This is the most straightforward way. It mimics ordinary transfer. To be able to call this action you will have to add additional permissions to your account as described in section two of *How to setup your account with proxy*.

code sample:
```C++
void transfer_via_proxy(account_name proxy, account_name from, account_name to, eosio::extended_asset quantity, std::string memo)
{
    eosio::dispatch_inline(proxy,  N(transfer), {{_self, N(active)}}, 
        std::make_tuple(_self, to, std::move(quantity), std::move(memo))
    );
}
....
transfer_via_proxy(proxy, from, to, extended_asset(eos_amount, N(eosio.code)), "memo");
```
See also [proxy_test.cpp#L82](src/test/proxy_test.cpp#L82).


cleos example:  
```
    cleso push action proxycontract transfer \
        '["sender", "recipient", {"quantity":"15.0000 EOS","contract":"eosio.token"}, "ransfer via cleos"]' -p sender
```

#### Token transfer to proxy contract
To make proxy transfer put proxy account name as recipient and write the name of actual recipient of token at the beginning of memo. This way the proxy dapp will be able to forward your transfer to recipient. If you want to include your own memo in the transfer you have to separate recipient name and your memo with one `space`.

  `cleos transfer <your_account> <proxy_account> <token_amount> "<token_recipient_name> <your_memo>"`   
  
In case you will be making proxy transfers from contract you can use helper function `gen_proxy_memo` located in the [utiles.hpp](src/utils.hpp#L25) file which can generate correct memo for you.

**Note**: If you're making token transfer to recipient who doesn't have already reserved balance space in token's contract you have to add additional permisions to your account.

### Mainnet
On the **EOS Mainnet** you can find [`transfer_proxy`](src/transfer_proxy.cpp) contract at [***proxytransax***](https://bloks.io/account/proxytransax) account.

### Testnet
On the [**EOS Jungle testnet**](http://jungle.cryptolions.io/) you can find [`transfer_proxy`](src/transfer_proxy.cpp) contract at [***proxytransax***](https://jungle.bloks.io/account/proxytransax) account. There is also [`proxy_test`](src/test/proxy_test.cpp) contract at [***tranproxytst***](https://bloks.io/account/tranproxytst) account.

### Improvements over other proxy contracts

This smartcontract is inspired by https://github.com/EOSEssentials/EOS-Proxy-Token, but extends it with;   
• Token transfers to the recipients that don't hold any yet.  
• Keeping your memo structure intact (easier integration).

#### The contract charges 0.1% fee on every transfer. Contact us for whitelisting. 

#### Additional code reviews welcome!


