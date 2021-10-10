import pandas
import matplotlib.pyplot as plt

file_path = 'model_weight_diff.csv'
df = pandas.read_csv(file_path, index_col=0, header=0)

print(df)

x = df.index
accuracy_df_len = len(df)
plt.figure(figsize=(20, 10))
for col in df.columns:
    plt.plot(x, df[col], label=col, alpha=0.75)
plt.grid()
plt.legend(ncol=5)
plt.title('model weight diff')
plt.xlabel('time (tick)')
plt.ylabel('weight diff')
plt.yscale('log')

plt.rcParams['svg.fonttype'] = 'none'
plt.savefig('model_weight_diff.pdf')
plt.savefig('model_weight_diff.jpg', dpi=800)
