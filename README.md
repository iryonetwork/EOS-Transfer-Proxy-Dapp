# EOS-Transfer-Proxy-Dapp
What this is
Put this contract onto an account with no available RAM and then use it to proxy token transfers from your dapp to users that might have code installed.

The problem
A malicious user can install code on their account which will allow them to insert rows in the name of another account sending them tokens. This lets them lock up RAM by inserting large amounts of garbage into rows when dapps/users send them tokens.

The solution
By sending tokens to a proxy account with no available RAM, and with a memo where the first word of the memo is the account you eventually want to send the tokens to, the only account they can assume database row permissions for is the proxy, which has no RAM.

Token types
This contract accepts all token types that conform to the basic eosio.token contract. The only method that has to have an identical argument signature is the transfer method.

#### EOS-Transfer-Proxy-Dapp is not production ready (needs more tests and reviews)!
