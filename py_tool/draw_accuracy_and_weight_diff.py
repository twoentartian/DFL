import pandas
import matplotlib.pyplot as plt

accuracy_file_path = 'accuracy.csv'
accuracy_df = pandas.read_csv(accuracy_file_path, index_col=0, header=0)

print(accuracy_df)

accuracy_x = accuracy_df.index
accuracy_df_len = len(accuracy_df)

weight_diff_file_path = 'model_weight_diff.csv'
weight_diff_df = pandas.read_csv(weight_diff_file_path, index_col=0, header=0)

print(weight_diff_df)

weight_diff_x = weight_diff_df.index
weight_diff_df_len = len(weight_diff_df)

fig, axs = plt.subplots(2, figsize=(10, 10))

for col in accuracy_df.columns:
    axs[0].plot(accuracy_x, accuracy_df[col], label=col, alpha=0.75)

for col in weight_diff_df.columns:
    axs[1].plot(weight_diff_x, weight_diff_df[col], label=col)

axs[0].grid()
axs[0].legend(ncol=5)
axs[0].set_title('accuracy')
axs[0].set_xlabel('time (tick)')
axs[0].set_ylabel('accuracy (0-1)')
axs[0].set_xlim([0, accuracy_df.index[accuracy_df_len-1]])
axs[0].set_ylim([0, 1])

axs[1].grid()
axs[1].legend()
axs[1].set_title('model weight diff')
axs[1].set_xlabel('time (tick)')
axs[1].set_ylabel('weight diff')
axs[1].set_yscale('log')
axs[1].set_xlim([0, weight_diff_df.index[weight_diff_df_len-1]])

plt.tight_layout()
plt.savefig('accuracy_weight_diff_combine.pdf')
plt.savefig('accuracy_weight_diff_combine.jpg', dpi=800)
