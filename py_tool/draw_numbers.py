import matplotlib.pyplot as plt

filename = ['/home/tyd/git/DFL/cmake-build-debug/tests/caffe_startup/node0blob1',
            '/home/tyd/git/DFL/cmake-build-debug/tests/caffe_startup/node0blob3',
            '/home/tyd/git/DFL/cmake-build-debug/tests/caffe_startup/node0blob5',
            '/home/tyd/git/DFL/cmake-build-debug/tests/caffe_startup/node0blob7']

if __name__ == '__main__':
    for file_path in filename:
        f = open(file_path)
        lines = f.readlines()
        X,Y = [],[]
        for line in lines:
            value = [float(s) for s in line.split()]
            X.append(value[0])
            Y.append(value[1])
        plt.figure()
        plt.plot(X, Y)
        plt.show()
