from config import action_bit_encoding
import json
from bitarray import bitarray
import pickle
import blosc

file_name = 'data_files/p2_valid_histories_for_reachable_I.json'

with open(file_name, 'r') as f:
    valid_histories_for_I = json.load(f)

for I_hash in valid_histories_for_I.keys():
    for i in range(len(valid_histories_for_I[I_hash])):
        b = bitarray()
        b.encode(action_bit_encoding, valid_histories_for_I[I_hash][i])
        valid_histories_for_I[I_hash][i] = b

pickled_data = pickle.dumps(valid_histories_for_I)
compressed_data = blosc.compress(pickled_data)

# save in compressed pickle format
with open(file_name[:-4] + '.dat', 'wb') as f:
    f.write(compressed_data)

