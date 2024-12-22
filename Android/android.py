import os
import random
import requests
from bitcoin import *
from concurrent.futures import ThreadPoolExecutor, as_completed
from colorama import Fore, Style, init
from tqdm import tqdm
import logging
import time

# Initialize colorama
init(autoreset=True)

# Logging settings
logging.basicConfig(level=logging.INFO, filename='bitcoin_check.log', filemode='a',
                    format='%(asctime)s - %(levelname)s - %(message)s')

# Randomly generate a Bitcoin private key
def generate_private_key():
    return random_key()

# Convert private key to bitcoin address
def private_key_to_address(private_key):
    public_key = privtopub(private_key)
    address = pubtoaddr(public_key)
    return address

# Address inventory check using BlockCypher API
def check_balance(address, token):
    url = f"https://api.blockcypher.com/v1/btc/main/addrs/{address}/balance?token={token}"
    response = requests.get(url)
    if response.status_code == 200:
        balance_data = response.json()
        return balance_data['final_balance']
    else:
        logging.error(f"Error checking balance for address {address}: {response.status_code}")
        return 0

# Save addresses that have inventory in a .txt file
def save_address_to_file(address, balance, filename="bitcoin_addresses.txt"):
    with open(filename, "a") as file:
        file.write(f"Address: {address}, Balance: {balance} satoshis\n")

# Enter your BlockCypher token here
blockcypher_token = 
3bc5b3d064c84fb7a26124fd43b0481f
# The number of private keys you want to generate
num_keys_to_generate = 1000000000

def process_key():
    private_key = generate_private_key()
    address = private_key_to_address(private_key)
    balance = check_balance(address, blockcypher_token)
    
    # Output the private key, address, and balance with color
    balance_btc = balance / 1e8  # Convert Satoshi to Bitcoin
    print(f"{Fore.BLUE}{private_key} -> {Fore.GREEN}{address} {Fore.YELLOW}(Balance: {balance_btc} BTC)")
    logging.info(f"Generated address: {address} with private key: {private_key} and balance: {balance_btc} BTC")
    
    if balance > 0:
        save_address_to_file(address, balance, filename="balance.txt")
        print(f"{Fore.RED}Address with balance found: {address} - Balance: {balance_btc} BTC")
        logging.info(f"Address with balance found: {address} - Balance: {balance_btc} BTC")
    else:
        save_address_to_file(address, balance)

# Using ThreadPoolExecutor for parallelization
with ThreadPoolExecutor(max_workers=10) as executor:
    futures = [executor.submit(process_key) for _ in range(num_keys_to_generate)]
    with tqdm(total=num_keys_to_generate, desc="Processing keys", unit=" key") as pbar:
        while True:
            finished = sum(future.done() for future in futures)
            pbar.update(finished - pbar.n)
            time.sleep(5)  # The status is updated every 5 seconds
            if all(future.done() for future in futures):
                break

print("Finished checking addresses.")