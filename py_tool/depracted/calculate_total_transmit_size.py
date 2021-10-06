import re

filename = "./test_data/drop_rate.txt"

if __name__ == '__main__':
    total_transmit_size = 0
    for line in open(filename):
        numbers = re.findall('\d+', line)
        total_transmit_size = total_transmit_size + int(numbers[-1])

    print(total_transmit_size)
