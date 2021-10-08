import pandas
import matplotlib.pyplot as plt

accuracy_file_path = 'accuracy.csv'
accuracy_df = pandas.read_csv(accuracy_file_path, index_col=0, header=0)

print(accuracy_df)

x = accuracy_df.index
accuracy_df_len = len(accuracy_df)
plt.figure(figsize=(20, 10))
for col in accuracy_df.columns:
    plt.plot(x, accuracy_df[col], label=col, alpha=0.75)
plt.grid()
plt.legend(ncol=5)
plt.title('accuracy')
plt.xlabel('time (tick)')
plt.ylabel('accuracy (0-1)')
plt.axis([0, accuracy_df.index[accuracy_df_len-1], 0, 1])

plt.rcParams['svg.fonttype'] = 'none'
plt.savefig('accuracy.pdf')
plt.savefig('accuracy.jpg', dpi=800)
