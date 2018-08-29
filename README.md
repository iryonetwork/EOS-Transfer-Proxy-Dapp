# RAM Token Proxy
is a  EOS Transfer Proxy Dapp

#### What does it do
Put this contract onto an account with no available RAM and then use it to proxy token transfers from your dapp to users that might have code installed.

#### The problem
A malicious user can install code on their account which will allow them to insert rows in the name of another account sending them tokens. This lets them lock up RAM by inserting large amounts of garbage into rows when dapps/users send them tokens.

#### The solution
By sending tokens to a proxy account with no available RAM, and with a memo where the first word of the memo is the account you eventually want to send the tokens to, the only account they can assume database row permissions for is the proxy, which has no RAM.

#### Token types
This contract accepts all token types that conform to the basic eosio.token contract. The only method that has to have an identical argument signature is the transfer method.

## Improvements over other proxy contracts

This smartcontract is inspired by https://github.com/EOSEssentials/EOS-Proxy-Token, but with an extended ability to allow transfers of the new tokens to the accounts.

Your contract needs to authorise the eosio.code to allow proxycontract to buy the ram in its name (on each transfer). Make sure you have enough EOS on contracts, otherwise the buy ram action (and whole transfer) would fail.


#### EOS-Transfer-Proxy-Dapp is not production ready (needs more tests and reviews)!


