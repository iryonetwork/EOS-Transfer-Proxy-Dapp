# Transfer Proxy dapp

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
  
2. In case your contract transfers other tokens than EOS token you have to add eosio.code permission and authorise proxycontract to the active permissions of your contract. This will allow the proxycontract to buy ram in case needed for the token transfer. Make sure you have enough EOS on contracts, otherwise the buy ram action (and whole transfer) will fail.

    ```
    cleos set account permission <your_account> active \
    '{"threshold": 1,"keys": [{"key": "<your_account_public_key>","weight": 1}],"accounts": [{"permission":{"actor":"<your_account>","permission":"eosio.code"},"weight":1},{"permission":{"actor":"<proxy_contract_account>","permission":"active"},"weight":1}]}' owner
    ```
### How to make transfer via proxy
After you have set up your account with the proxy contract you are ready to make token transfer via proxy.
To make proxy transfer put proxy account name as recipient and write the name of actual recipient of token at the beginning of memo. This way the proxy dapp will be able to forward your transfer to recipient. If you want to include your own memo in the transfer you have to separate recipient name and your memo with one `space`.

  `cleos transfer <your_account> <proxy_account> <token_amount> "<token_recipient_name> <your_memo>"`
  
In case you will be making proxy transfers from contract you can use helper function `gen_proxy_memo` located in the [utiles.hpp](https://github.com/iryonetwork/RAM-Token-Proxy/blob/master/src/utils.hpp#L25) file which can generate correct memo for you.


### Improvements over other proxy contracts

This smartcontract is inspired by https://github.com/EOSEssentials/EOS-Proxy-Token, but with an extended ability to allow transfers of the new tokens to the accounts.

#### The contract charges 0.1% fee on every transfer. Contact us for whitelisting. 

#### EOS-Transfer-Proxy-Dapp is not yet production ready (needs more tests and reviews)!


