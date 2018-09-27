#!/bin/bash
TX_TTL=${TX_TTL:-"30"} # 30s
CLEOS=${CLEOS:-"cleos"}

########################################################################

function print_help {
    echo "This script registers an account with transfer proxy contract."
    echo "Optionally an additional account permissions will be set."
    echo -e "\nUsage: <account><proxy_contract>[account_proxy_setup_acion]"
}

# Check num param
if [ "$#" -lt 2 ]; then
    print_help
    exit 1
fi

# Parse args
ACCOUNT=$1
PROXY=$2

ABI_ACTION_SIGNUP="signup"
ABI_ACTION_SETUP_PROXY=
if [ "$#" -ge 3 ]; then
    ABI_ACTION_SETUP_PROXY=$3
fi


# Signup account with proxy
echo "Signing up '$ACCOUNT' with proxy '$PROXY' ..."
RESULT=$($CLEOS push action -x$TX_TTL $PROXY $ABI_ACTION_SIGNUP '["'$ACCOUNT'"]' -p $ACCOUNT@active 2>&1)
rc=$?; if [[ $rc != 0 ]]; then (1>&2 echo "$RESULT"); exit $rc; fi

# Add proxy to contract
if [ -n "$ABI_ACTION_SETUP_PROXY" ]; then
    RESULT=$($CLEOS push action -x$TX_TTL $ACCOUNT $ABI_ACTION_SETUP_PROXY '["'$PROXY'"]' -p $ACCOUNT@active 2>&1)
    rc=$?; if [[ $rc != 0 ]]; then (1>&2 echo "$RESULT"); exit $rc; fi
fi


# Add permissions to account
echo -e "\nAdd $PROXY@eosio.code permission to $ACCOUNT?"

ACCOUNT_PROXY_PERM=""
N_PERMS=0
select yn in "Yes" "No" ; do
    case $yn in
        "Yes" ) 
            ACCOUNT_PROXY_PERM='{"permission":{"actor":"'$PROXY'","permission":"eosio.code"},"weight":1}';
            N_PERMS=$((N_PERMS+1))
            break
            ;;
        "No" ) break ;;
    esac
done

if type "jqt" > /dev/null 2>&1 ; then
    if [[ $N_PERMS > 0 ]] ; then
        ACTIVE_PERM=$($CLEOS get account -j tranproxytst 2>&1)
        rc=$?; if [[ $rc != 0 ]]; then (1>&2 echo "$RESULT"); exit $rc; fi
        ACTIVE_PERM=$(jq -n "$ACTIVE_PERM" | jq '.permissions[] | select(.perm_name == "active") | .required_auth' 2>&1)

        if [[ -z $ACTIVE_PERM ]] ; then
            echo "Error: No active permission level found on account '$ACCOUNT'!"
            exit 1
        fi

        if [[ -z $(jq -n "$ACTIVE_PERM" | jq '.accounts[] | select(.permission.actor == "'$PROXY'")' 2>&1) ]] ; then
            NEW_ACTIVE_PERMS=$(jq -n "$ACTIVE_PERM" | jq \
                '.accounts += [{"permission":{"actor":"'$PROXY'","permission":"eosio.code"},"weight":1}] |
                 .accounts = (.accounts | sort_by(.permission.actor))'  2>&1)
        fi
    fi
else # fallback
    echo -e "\nAdd $ACCOUNT@eosio.code permission to $ACCOUNT?"
    ACCOUNT_CODE_PERM=""
    select yn in "Yes" "No" ; do
        case $yn in
            "Yes" ) 
                ACCOUNT_CODE_PERM='{"permission":{"actor":"'$ACCOUNT'","permission":"eosio.code"},"weight":1}';
                N_PERMS=$((N_PERMS+1))
                break
                ;;
            "No" ) break ;;
        esac
    done

    if [[ $N_PERMS > 0 ]]; then
        echo -e "\nEnter '$ACCOUNT' active permission public key: "
        read PUB_KEY

        ACNT_1=$ACCOUNT_CODE_PERM
        ACNT_2=$ACCOUNT_PROXY_PERM
        if [[ "$ACNT_1" > "$ACNT_2" ]] ; then # sort
            ACNT_1=$ACCOUNT_PROXY_PERM
            ACNT_2=$ACCOUNT_CODE_PERM
        fi

        SEP=""; if [[ $N_PERMS == 2 ]]; then SEP=","; fi
        NEW_ACTIVE_PERMS='{"threshold": 1,"keys": [{"key": "'$PUB_KEY'","weight": 1}],
            "accounts": ['${ACNT_1}${SEP}${ACNT_2}']}'
    fi
fi

if [[ $NEW_ACTIVE_PERMS != "" ]] ; then
    echo -e "\nAdding additional permissions to '$ACCOUNT' ..."
    RESULT=$($CLEOS set 'account permission' $ACCOUNT active "$NEW_ACTIVE_PERMS" owner -p $ACCOUNT@active 2>&1)
    rc=$?; if [[ $rc != 0 ]]; then (1>&2 echo "$RESULT"); exit $rc; fi
fi


# Success
echo "Proxy was successfully setup!"
