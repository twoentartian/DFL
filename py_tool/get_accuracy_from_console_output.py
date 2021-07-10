import re

file_path = 'test_data/console_output.txt'
number_of_node = 4

if __name__ == '__main__':
    f = open(file_path)
    lines = f.readlines()
    count = 0
    for line in lines:
        if line.find('accuracy') != -1:
            accuracy = re.findall(r'\d*\.\d+', line)
            print(accuracy[0], end='')
            print(' ', end='')
            count = count + 1
            if count == number_of_node:
                count = 0
                print('')