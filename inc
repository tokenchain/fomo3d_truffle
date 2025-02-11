#!/usr/bin/env bash
#NVM_DIR=~/.nvm
#. ~/.nvm/nvm.sh
COMPRESSED_NAME="solc-build.tar.gz"
# the files to compile
#

help() {
  local usage=" Explorer builder autoscript -h\n
	 Please try select any of these cmd - testnet,dx1\n

	 example\n

	 	testnet: sh build.sh testnet\n
	 production: sh build.sh dx1\n

	 skip upload: sh build.sh testnet -test\n
	 skip upload: sh build.sh dx1  -test\n
	 	try to help it out
	 "
  echo $usage
}

abort_program() {
  cd $BUILD_DIR
  rm -f $FILE
  exit
}

rename_pyc_to_regular() {
  local path=$1
  local build="pycbuild"

  python3 -c "$_c"
}

#1: the full path
#2: the target location in the remote server
upload_file() {
  local file_size_kb=$(du -k "$1" | cut -f1)

  if [[ $file_size_kb -eq 0 ]]; then
    echo "⛔️ file is zero bytes..."
    abort_program
  fi

  scp $1 root@$LOCAL:$2

  if [ $? -eq 0 ]; then
    echo "✅ ==== upload successfully"
  # else
  #echo "⛔️ Error from uploading... $1"
  # abort_program
  fi

}

remotecmd() {
  local remote_cmd=$2
  ssh -t root@$LOCAL "cd $TARGET_LOC; bash; $remote_cmd"
}

directcmd() {
  ssh -t root@$LOCAL "cd $TARGET_LOC; ./build.sh"
}

mod_setting() {
  param_hk="$1 = \"$2\""
  #echo "$param_hk"
  cat $EXPLORER_SETTINGS | jq "$param_hk" -c $EXPLORER_SETTINGS | sponge $EXPLORER_SETTINGS
}

env_segment() {
  if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "mainnet"
  elif [[ "$OSTYPE" == "darwin"* ]]; then
    # Mac OSX
    echo "testlocal"
  elif [[ "$OSTYPE" == "cygwin" ]]; then
    echo "testlocal"
    # POSIX compatibility layer and Linux environment emulation for Windows
  elif [[ "$OSTYPE" == "msys" ]]; then
    # Windows
    echo "testlocal"
  elif [[ "$OSTYPE" == "freebsd"* ]]; then
    # ...
    echo "testlocal"
  fi
}

env_setting_file() {
  if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "settings_centos.json"
  elif [[ "$OSTYPE" == "darwin"* ]]; then
    # Mac OSX
    echo "settings_local.json"
  elif [[ "$OSTYPE" == "cygwin" ]]; then
    echo "settings_local.json"
    # POSIX compatibility layer and Linux environment emulation for Windows
  elif [[ "$OSTYPE" == "msys" ]]; then
    # Windows
    echo "settings_local.json"
  elif [[ "$OSTYPE" == "freebsd"* ]]; then
    # ...
    echo "settings_local.json"
  fi
}

linuxtools() {
  /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
  #https://snapcraft.io/install/solc/centos
  sudo yum install rsync
}



flaten(){
cd ./vault
solflatliner --o TetherTokenLZ.sol USDT/LayerZeroUSDT.sol 0.6.6
}

build_abi_generation() {
  local _c=$(
    cat <<EOF

# !/usr/bin/env python
# coding: utf-8
import os
from moody import conf, Evm
from moody.libeb import MiliDoS

NETWORK = conf.RSCMainnet()
#SOLV = "0.6.12"
SOLV = "0.8.6"
CONTRACT_LIST = [
    "vault/newfomo/fomo3d.sol"
]

dos = MiliDoS(NETWORK)
print("-----> to verify this sale now")
print("$BUILDPATH")
print("-----> this is the end of the path")
dos.setWorkspace("$BUILDPATH").setClassSolNames(CONTRACT_LIST).setEvm(Evm.ISTANBUL).remoteCompile(SOLV).localTranspile()


EOF
  )
  cd $BUILDPATH
  python3 -c "$_c"
}


build_contract_deploy() {
  # LayerZeroEndpoint
  local _endpoint=""

  local _private_key=$(cat pkey.txt)
  local _c=$(
    cat <<EOF

# !/usr/bin/env python
# coding: utf-8
# from web3.auto import w3
from moody.libeb import MiliDoS
from web3 import Web3, HTTPProvider

NETWORK = conf.RSCMainnet()
dos = MiliDoS(NETWORK).withPOA()
dos.setWorkspace("$BUILDPATH").Auth("$_private_key")
# RSC configuration
dos.OverrideGasConfig(6000000, 2000000000)
# BSC configuration
# dos.OverrideGasConfig(6000000, 5500000000)

dos.deploy("BlindBoxSale", ["$_endpoint"])


EOF
  )
  cd $BUILDPATH
  python3 -c "$_c"
}
#--combined-json abi,asm,ast,bin,bin-runtime,compact-format,devdoc,hashes,interface,metadata,opcodes,srcmap,srcmap-runtime,userdoc
mactools() {
  # -----------------------------------------------
  if ! command -v cnpm &>/dev/null; then
    echo "cnpm could not be found"
    npm i -g cnpm
  fi
  # -----------------------------------------------
  if ! command -v rsync &>/dev/null; then
    echo "rsync could not be found"
    brew install rsync
  fi
  # -----------------------------------------------
  if ! command -v abi-gen-uni &>/dev/null; then
    echo "abi-gen-uni could not be found. please check the official source from: https://www.npmjs.com/package/easy-abi-gen"
    cnpm i -g easy-abi-gen
  fi
  # -----------------------------------------------
  if ! command -v abigen &>/dev/null; then
    echo "golang abigen could not be found"
    exit
  fi
  #NVM_VERSION=$(echo "$(node -v)" | grep -o -E '[0-9]{2}.')
  local NVM_VERSION=$(echo "$(node -v)" | cut -d. -f1)
  echo "==> 🈯️ all modules needed are completed."
  # -----------------------------------------------
  if [[ ${NVM_VERSION} == "v12" ]]; then
    echo "node version is on the right version : v12"
  else
    echo "please use the below command to switch to the right version of node"
    echo "nvm use 12"
    exit
  fi
  # -----------------------------------------------
  python3 -m compileall .
}


endline() {
  cat remotesolc | ssh root@$LOCAL /bin/bash
  echo "==> 🈯️ download file from remote $TARGET_LOC"
  if [[ ! -f $BUILDPATH/build ]]; then
    mkdir -p $BUILDPATH/build
  fi
  cd $BUILDPATH
  cp -R $FACTORY $BUILDPATH/factoryabi
  rsync -avzhe ssh root@$LOCAL:$TARGET_LOC/$COMPRESSED_NAME $BUILDPATH
  rm remotesolc
  mv $BUILDPATH/$COMPRESSED_NAME $BUILDPATH/build
  cd "$BUILDPATH/build"
  tar -xvf $COMPRESSED_NAME
  rm $COMPRESSED_NAME
  echo "==> 🛃 solc process completed."
  cd $BUILDPATH
  sh localpile
  rm localpile
  rm -rf $BUILDPATH/factoryabi
  echo "==> 🛃 local transpile process completed."
}

deploy_py_backend() {
  python3 -m compileall .
  local pycfolder="$PWD/__pycache__"
  #rename_pyc_to_regular $pycfolder
}

# Accepts a version string and prints it incremented by one.
# Usage: increment_version <version> [<position>] [<leftmost>]
increment_version() {
  declare -a part=(${1//\./ })
  declare new
  declare -i carry=1

  for ((CNTR = ${#part[@]} - 1; CNTR >= 0; CNTR -= 1)); do
    len=${#part[CNTR]}
    new=$((part[CNTR] + carry))
    [ ${#new} -gt $len ] && carry=1 || carry=0
    [ $CNTR -gt 0 ] && part[CNTR]=${new: -len} || part[CNTR]=${new}
  done

  new="${part[*]}"

  if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo -e "${new// /.}"
  elif [[ "$OSTYPE" == "darwin"* ]]; then
    echo "${new// /.}"
  elif [[ "$OSTYPE" == "cygwin" ]]; then
    echo "not correct system - cygwin detected"
    exit
  fi

}

preinstall() {
  npm install -g dts-gen
  npm install typescript --save-dev
  pip3 install git+git://github.com/psf/black
}

mod_package_json() {
  param_chan=$(echo "$1 = \"$2\"")
  echo "$param_chan"
  cat $NODEPFILE | jq "$param_chan" $NODEPFILE | sponge $NODEPFILE
}

auto_install_nvm() {
  # shellcheck disable=SC2077
  if [[ $auto_install == 1 ]]; then
    $CMD_FINAL
  fi
}

gitconnectiontest() {
  ssh -Tvvv git@gitlab.com
  ssh -T git@gitlab.com
}

fixmessedupgit() {
  git rm -r --cached .
  git add .
  git commit -am 'git cache cleared'
  git push
}

gitpush() {
  git config pull.rebase true
  git add .
  git commit -m "Check Point 🍥 ${gitcheck}"
  git push origin
  echo "♻️ You can open from the list of url as shown below"
  git remote -v
}

backup_deploy() {
  cd $BUILDPATH
  declare folder_name="deploy_history/v$(cat version)"
  declare folder_build="deploy_history/v$(cat version)/build"
  #this is awesome and already to go now
  if [[ ! -f $folder_name ]]; then
    mkdir -p "$folder_name"
    mkdir -p "$folder_build"
  fi
  cp deploy_results/*.* $folder_name
  cp build/*.* $folder_build
}
# Please setup your local path for local transpiling
# LOCAL MACHINE ONLY the default path
BUILDPATH="$HOME/Documents/b95/fomo3d/"
BUILD_DIR="$BUILDPATH/build"
FACTORY=$HOME/Documents/piplines/factoryabi