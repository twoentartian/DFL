#pragma once

#include <glog/logging.h>

#include <boost/shared_ptr.hpp>

#include <caffe/layers/memory_data_layer.hpp>
#include <caffe/solver.hpp>
#include <caffe/common.hpp>

#include "tensor_blob_like.hpp"

namespace Ml
{
	template <typename DType, template <typename> class SolverType>
	class caffe_solver_ext : public SolverType<DType>
	{
	public:
		explicit caffe_solver_ext(const caffe::SolverParameter& param) : SolverType<DType>(param){}
		
		explicit caffe_solver_ext(const std::string& param_file) : SolverType<DType>(param_file){}
		
		void ApplyUpdate() override
		{
			SolverType<DType>::ApplyUpdate();
		}
		
		void SnapshotSolverState(const std::string& model_filename) override
		{
			SolverType<DType>::SnapshotSolverState(model_filename);
		}
		
		void RestoreSolverStateFromHDF5(const std::string& state_file) override
		{
			SolverType<DType>::RestoreSolverStateFromHDF5(state_file);
		}
		
		void RestoreSolverStateFromBinaryProto(const std::string& state_file) override
		{
			SolverType<DType>::RestoreSolverStateFromBinaryProto(state_file);
		}
		
		void TrainDataset(const std::vector<tensor_blob_like<DType>>& data, const std::vector<tensor_blob_like<DType>>& label, bool display = true)
		{
			CHECK(!data.empty()) << "empty train data, data size == 0";
			{
				int data_length = 1;
				for (auto&& dimension: data[0].getShape())
				{
					data_length *= dimension;
				}
				CHECK(data_length == data[0].getData().size()) << "data shape and data size mismatch";
			}
			CHECK(data.size() == label.size()) << "data size does not equal label size";
			CHECK(checkValidFirstLayer_memoryLayer()) << "the first layer is not MemoryData layer";
			
			auto first_layer_abs = this->net().get()->layers()[0];
			auto first_layer = boost::dynamic_pointer_cast<caffe::MemoryDataLayer<DType>>(first_layer_abs);
			auto batch_size = first_layer->batch_size();
			
			if (data.size() % batch_size != 0)
			{
				LOG(WARNING) << "data size is not a multiple of batch size";
			}
			
			int current_train_index = 0;
			std::vector<tensor_blob_like<DType>> iter_data,iter_label;
			iter_data.resize(batch_size);iter_label.resize(batch_size);
			
			
			int average_loss = this->param_.average_loss();
			this->losses_.clear();
			this->smoothed_loss_ = 0;
			this->iteration_timer_.Start();
			
			while (current_train_index < data.size())
			{
				//fill MemoryDataLayer
				iter_data.clear();
				iter_label.clear();
				int remain_size = data.size() - current_train_index;
				int iters = std::min(remain_size, batch_size);
				for (int i = 0; i < iters; i++)
				{
					iter_data.push_back(data[current_train_index]);
					iter_label.push_back(label[current_train_index]);
					current_train_index++;
				}
				auto datums = ConvertTensorBlobLikeToDatum(iter_data,iter_label);
				first_layer->AddDatumVector(datums);
				
				TrainDataset_Step(1, average_loss, display);
			}
		}
		
		//return: vector<accuracy,loss>, vector.size = #test nets
		std::vector<std::tuple<DType,DType>> TestDataset(const std::vector<tensor_blob_like<DType>>& data, const std::vector<tensor_blob_like<DType>>& label)
		{
			CHECK(!data.empty()) << "empty test data, data size == 0";
			{
				int data_length = 1;
				for (auto&& dimension: data[0].getShape())
				{
					data_length *= dimension;
				}
				CHECK(data_length == data[0].getData().size()) << "data shape and data size mismatch";
			}
			CHECK(data.size() == label.size()) << "data size does not equal label size";
			CHECK(checkValidFirstLayer_memoryLayer()) << "the first layer is not MemoryData layer";
			
			const std::vector<boost::shared_ptr<caffe::Net<DType>>>& test_nets = this->test_nets();
			std::vector<std::tuple<DType,DType>> output;
			output.reserve(test_nets.size());
			for (int test_nets_index = 0; test_nets_index < test_nets.size(); test_nets_index++)
			{
				auto first_layer_abs = test_nets[test_nets_index]->layers()[0];
				auto first_layer = boost::dynamic_pointer_cast<caffe::MemoryDataLayer<DType>>(first_layer_abs);
				
				//test batch size will not work, we will test all test data.
				//auto batch_size = first_layer->batch_size();
				
				auto datums = ConvertTensorBlobLikeToDatum(data,label);
				first_layer->AddDatumVector(datums);
				output.push_back(TestDataset_Step(test_nets_index));
			}
			return output;
		}
		
		bool checkValidFirstLayer_memoryLayer()
		{
			auto& test_first_layer = this->test_nets().data()->get()->layers()[0];
			auto& first_layer = this->net()->layers()[0];
			std::string test_type(test_first_layer->type());
			std::string type(first_layer->type());
			return (type == "MemoryData") && (test_type == "MemoryData");
		}
		
	private:
		void TrainDataset_Step(int iter, int average_loss, bool display = true)
		{
			const int start_iter = this->iter_;
			const int stop_iter = this->iter_ + iter;
			
			while (this->iter_ < stop_iter)
			{
				// zero-init the params
				this->net_->ClearParamDiffs();
				for (int i = 0; i < this->callbacks_.size(); ++i)
				{
					this->callbacks_[i]->on_start();
				}
				//const bool display = param_.display() && iter_ % param_.display() == 0;
				this->net_->set_debug_info(display && this->param_.debug_info());
				// accumulate the loss and gradient
				DType loss = 0;
				for (int i = 0; i < this->param_.iter_size(); ++i)
				{
					loss += this->net_->ForwardBackward();
				}
				loss /= this->param_.iter_size();
				// average the loss across iterations for smoothed reporting
				this->UpdateSmoothedLoss(loss, start_iter, average_loss);
				if (display)
				{
					float lapse = this->iteration_timer_.Seconds();
					float per_s = (this->iter_ - this->iterations_last_) / (lapse ? lapse : 1);
					LOG_IF(INFO, caffe::Caffe::root_solver()) << "Iteration " << this->iter_ << " (" << per_s << " iter/s, " << lapse << "s/" << this->param_.display() << " iters), loss = " << this->smoothed_loss_;
					this->iteration_timer_.Start();
					this->iterations_last_ = this->iter_;
					const std::vector<caffe::Blob<DType>*>& result = this->net_->output_blobs();
					int score_index = 0;
					for (int j = 0; j < result.size(); ++j)
					{
						const DType* result_vec = result[j]->cpu_data();
						const std::string& output_name = this->net_->blob_names()[this->net_->output_blob_indices()[j]];
						const DType loss_weight = this->net_->blob_loss_weights()[this->net_->output_blob_indices()[j]];
						for (int k = 0; k < result[j]->count(); ++k)
						{
							std::ostringstream loss_msg_stream;
							if (loss_weight)
							{
								loss_msg_stream << " (* " << loss_weight << " = " << loss_weight * result_vec[k] << " loss)";
							}
							LOG_IF(INFO, caffe::Caffe::root_solver()) << "    Train net output #" << score_index++ << ": " << output_name << " = " << result_vec[k] << loss_msg_stream.str();
						}
					}
				}
				for (int i = 0; i < this->callbacks_.size(); ++i)
				{
					this->callbacks_[i]->on_gradients_ready();
				}
				this->ApplyUpdate();
			}
		}
		
		//return: {accuracy,loss}
		std::tuple<DType,DType> TestDataset_Step(const int test_net_id)
		{
			DType output_accuracy,output_loss;
			CHECK(caffe::Caffe::root_solver());
			LOG(INFO) << "Iteration " << this->iter_ << ", Testing net (#" << test_net_id << ")";
			CHECK_NOTNULL(this->test_nets_[test_net_id].get())->ShareTrainedLayersWith(this->net_.get());
			std::vector<DType> test_score;
			std::vector<int> test_score_output_id;
			const boost::shared_ptr<caffe::Net<DType> >& test_net = this->test_nets_[test_net_id];
			DType loss = 0;
			for (int i = 0; i < this->param_.test_iter(test_net_id); ++i)
			{
				DType iter_loss;
				const std::vector<caffe::Blob<DType>*>& result =	test_net->Forward(&iter_loss);
				if (this->param_.test_compute_loss()) {
					loss += iter_loss;
				}
				if (i == 0)
				{
					for (int j = 0; j < result.size(); ++j)
					{
						const DType* result_vec = result[j]->cpu_data();
						for (int k = 0; k < result[j]->count(); ++k)
						{
							test_score.push_back(result_vec[k]);
							test_score_output_id.push_back(j);
						}
					}
				}
				else
				{
					int idx = 0;
					for (int j = 0; j < result.size(); ++j)
					{
						const DType* result_vec = result[j]->cpu_data();
						for (int k = 0; k < result[j]->count(); ++k) {
							test_score[idx++] += result_vec[k];
						}
					}
				}
			}
			//TODO: do we need to test compute loss? maybe currently no
//			if (this->param_.test_compute_loss())
//			{
//				loss /= this->param_.test_iter(test_net_id);
//				LOG(INFO) << "Test loss: " << loss;
//			}
			for (int i = 0; i < test_score.size(); ++i)
			{
				const int output_blob_index = test_net->output_blob_indices()[test_score_output_id[i]];
				const std::string& output_name = test_net->blob_names()[output_blob_index];
				const DType loss_weight = test_net->blob_loss_weights()[output_blob_index];
				std::ostringstream loss_msg_stream;
				const DType mean_score = test_score[i] / this->param_.test_iter(test_net_id);
				if (loss_weight)
				{
					loss_msg_stream << " (* " << loss_weight << " = " << loss_weight * mean_score << " loss)";
				}
				LOG(INFO) << "    Test net output #" << i << ": " << output_name << " = " << mean_score << loss_msg_stream.str();
				
				if (output_name == "accuracy")
				{
					output_accuracy = mean_score;
				}
				else if(output_name == "loss")
				{
					output_loss = mean_score;
				}
				else
				{
					LOG(WARNING) << "Unknown output_name in test";
				}
			}
			return {output_accuracy,output_loss};
		}
		
		std::vector<caffe::Datum> ConvertTensorBlobLikeToDatum(const std::vector<tensor_blob_like<DType>>& data, const std::vector<tensor_blob_like<DType>>& label)
		{
			std::vector<caffe::Datum> output;
			output.resize(data.size());
			DType temp_label;
			const auto& shape = data[0].getShape();
			const int& single_sample_length = data[0].getData().size();
			DType* temp_pixels = new DType[single_sample_length];
			{
				for (int item_id = 0; item_id < data.size(); ++item_id)
				{
					output[item_id].set_channels(shape[0]);
					output[item_id].set_height(shape[1]);
					output[item_id].set_width(shape[2]);
					std::memcpy(temp_pixels,data[item_id].getData().data(), sizeof(DType) * data[item_id].getData().size());
					std::memcpy(&temp_label,label[item_id].getData().data(), sizeof(DType) * label[item_id].getData().size());
					output[item_id].set_data(temp_pixels, single_sample_length);
					output[item_id].set_label(temp_label);
				}
			}
			delete[] temp_pixels;
			return std::move(output);
		}
		
		
	};
}
