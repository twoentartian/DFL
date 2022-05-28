import os
import sys

import pandas
import matplotlib.pyplot as plt

row = 6
col = 4

folders = ["no_malicious_fedavg_10_iid", "no_malicious_fedavg_10_alpha1", "no_malicious_fedavg_10_alpha0_1", "no_malicious_fedavg_10_1node2label",
           "dataset_attack_x1_fedavg_10_iid", "dataset_attack_x1_fedavg_10_alpha1", "dataset_attack_x1_fedavg_10_alpha0_1", "dataset_attack_x1_fedavg_10_1node2label",
           "model_attack_fedavg_10_iid", "model_attack_fedavg_10_alpha1", "model_attack_fedavg_10_alpha0_1", "model_attack_fedavg_10_1node2label",
           "no_malicious_reputation_10_iid", "no_malicious_reputation_10_alpha1", "no_malicious_reputation_10_alpha0_1", "no_malicious_reputation_10_1node2label",
           "dataset_attack_x1_reputation_10_iid", "dataset_attack_x1_reputation_10_alpha1", "dataset_attack_x1_reputation_10_alpha0_1", "dataset_attack_x1_reputation_10_1node2label",
           "model_attack_reputation_10_iid", "model_attack_reputation_10_alpha1", "model_attack_reputation_10_alpha0_1", "model_attack_reputation_10_1node2label"
           ]

titles = ["no_malicious + iid", "no_malicious + alpha=1", "no_malicious + alpha=0.1", "no_malicious + 2labelsPerNode",
          "dataset_attack + iid", "dataset_attack + alpha=1", "dataset_attack + alpha=0.1", "dataset_attack + 2labelsPerNode",
          "model_attack + iid", "model_attack + alpha=1", "model_attack + alpha=0.1", "model_attack + 2labelsPerNode",
          "no_malicious + naive_reputation + iid", "no_malicious + naive_reputation + alpha=1", "no_malicious + naive_reputation + alpha=0.1", "no_malicious + naive_reputation + 2labelsPerNode",
          "dataset_attack + naive_reputation + iid", "dataset_attack + naive_reputation + alpha=1", "dataset_attack + naive_reputation + alpha=0.1", "dataset_attack + naive_reputation + 2labelsPerNode",
          "model_attack + naive_reputation + iid", "model_attack + naive_reputation + alpha=1", "model_attack + naive_reputation + alpha=0.1", "model_attack + naive_reputation + 2labelsPerNode",
          ]

folder_names_set = set()
for folder_index in range(len(folders)):
    folder = folders[folder_index]
    assert not (folder in folder_names_set)
    folder_names_set.add(folder)


def query_yes_no(question, default="yes"):
    """Ask a yes/no question via raw_input() and return their answer.

    "question" is a string that is presented to the user.
    "default" is the presumed answer if the user just hits <Enter>.
            It must be "yes" (the default), "no" or None (meaning
            an answer is required of the user).

    The "answer" return value is True for "yes" or False for "no".
    """
    valid = {"yes": True, "y": True, "ye": True, "no": False, "n": False}
    if default is None:
        prompt = " [y/n] "
    elif default == "yes":
        prompt = " [Y/n] "
    elif default == "no":
        prompt = " [y/N] "
    else:
        raise ValueError("invalid default answer: '%s'" % default)

    while True:
        sys.stdout.write(question + prompt)
        choice = input().lower()
        if default is not None and choice == "":
            return valid[default]
        elif choice in valid:
            return valid[choice]
        else:
            sys.stdout.write("Please respond with 'yes' or 'no' " "(or 'y' or 'n').\n")


assert len(folders) == row * col

flag_generate_whole = query_yes_no('do you want to generate the whole figure?')
if flag_generate_whole:
    whole_fig, whole_axs = plt.subplots(row*2, col, figsize=(10*col, 10*row))
    for folder_index in range(len(folders)):
        current_col = folder_index % col
        current_row = folder_index // col

        folder = folders[folder_index]

        subfolders = [f.path for f in os.scandir(folder) if f.is_dir()]
        assert len(subfolders) != 0
        final_accuracy_df = pandas.DataFrame()
        final_weight_diff_df = pandas.DataFrame()
        is_first_dataframe = True
        for each_test_result_folder in subfolders:
            accuracy_file_path = each_test_result_folder + '/accuracy.csv'
            accuracy_df = pandas.read_csv(accuracy_file_path, index_col=0, header=0)
        # print(accuracy_df)

            weight_diff_file_path = each_test_result_folder + '/model_weight_diff.csv'
            weight_diff_df = pandas.read_csv(weight_diff_file_path, index_col=0, header=0)
        # print(weight_diff_df)

            if is_first_dataframe:
                is_first_dataframe = False
                final_accuracy_df = accuracy_df
                final_weight_diff_df = weight_diff_df
            else:
                final_accuracy_df = final_accuracy_df.add(accuracy_df, fill_value=0)
                final_weight_diff_df = final_weight_diff_df.add(weight_diff_df, fill_value=0)
        final_accuracy_df = final_accuracy_df.div(len(subfolders))
        final_weight_diff_df = final_weight_diff_df.div(len(subfolders))
        print(final_accuracy_df)
        print(final_weight_diff_df)

        accuracy_x = final_accuracy_df.index
        accuracy_df_len = len(final_accuracy_df)
        weight_diff_x = final_weight_diff_df.index
        weight_diff_df_len = len(final_weight_diff_df)

        accuracy_axis = whole_axs[current_row*2, current_col]
        weight_diff_axis = whole_axs[current_row*2+1, current_col]

        for _col in final_accuracy_df.columns:
            accuracy_axis.plot(accuracy_x, final_accuracy_df[_col], label=_col, alpha=0.75)

        for _col in final_weight_diff_df.columns:
            weight_diff_axis.plot(weight_diff_x, final_weight_diff_df[_col], label=_col, linewidth=2)

        accuracy_axis.grid()
        accuracy_axis.legend(ncol=5)
        accuracy_axis.set_title('Subplot ' + str(folder_index) + 'a - accuracy: ' + titles[folder_index])
        accuracy_axis.set_xlabel('time (tick)')
        accuracy_axis.set_ylabel('accuracy (0-1)')
        accuracy_axis.set_xlim([0, final_accuracy_df.index[accuracy_df_len - 1]])
        accuracy_axis.set_ylim([0, 1])

        weight_diff_axis.grid()
        weight_diff_axis.legend()
        weight_diff_axis.set_title('Subplot ' + str(folder_index) + 'b - model weight diff: ' + titles[folder_index])
        weight_diff_axis.set_xlabel('time (tick)')
        weight_diff_axis.set_ylabel('weight diff')
        weight_diff_axis.set_yscale('log')
        weight_diff_axis.set_xlim([0, final_weight_diff_df.index[weight_diff_df_len - 1]])

    whole_fig.tight_layout()
    whole_fig.savefig('test.pdf')
    whole_fig.savefig('test.jpg', dpi=800)
    plt.close(whole_fig)

flag_generate_for_each_result = query_yes_no('do you want to draw accuracy graph and weight difference graph for each simulation result?')
if flag_generate_for_each_result:
    for folder_index in range(len(folders)):
        folder = folders[folder_index]
        subfolders = [f.path for f in os.scandir(folder) if f.is_dir()]
        assert len(subfolders) != 0
        for each_test_result_folder in subfolders:
            print("processing: " + each_test_result_folder)
            accuracy_file_path = each_test_result_folder + '/accuracy.csv'
            accuracy_df = pandas.read_csv(accuracy_file_path, index_col=0, header=0)

            weight_diff_file_path = each_test_result_folder + '/model_weight_diff.csv'
            weight_diff_df = pandas.read_csv(weight_diff_file_path, index_col=0, header=0)

            accuracy_x = accuracy_df.index
            accuracy_df_len = len(accuracy_df)

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
            axs[0].set_xlim([0, accuracy_df.index[accuracy_df_len - 1]])
            axs[0].set_ylim([0, 1])

            axs[1].grid()
            axs[1].legend()
            axs[1].set_title('model weight diff')
            axs[1].set_xlabel('time (tick)')
            axs[1].set_ylabel('weight diff')
            axs[1].set_yscale('log')
            axs[1].set_xlim([0, weight_diff_df.index[weight_diff_df_len - 1]])

            fig.tight_layout()
            fig.savefig(each_test_result_folder + '/accuracy_weight_diff_combine.pdf')
            fig.savefig(each_test_result_folder + '/accuracy_weight_diff_combine.jpg', dpi=800)
            plt.close(fig)