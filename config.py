from bitarray import bitarray

num_workers = 96
action_bit_encoding = {0: bitarray('0000'), 1: bitarray('0001'), 2: bitarray('0010'), 3: bitarray('0011'),
                        4: bitarray('0100'), 5: bitarray('0101'), 6: bitarray('0110'), 7: bitarray('0111'),
                        8: bitarray('1000'), 9: bitarray('1001'), 10: bitarray('1010'), 11: bitarray('1011'),
                        12: bitarray('1100')}