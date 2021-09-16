#pragma once

#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <set>
#include <cmath>
#include <cstring>
#include <limits>

namespace clustering
{
	template <typename DType, typename child_type>
	class data_point
	{
	public:
		using data_type = DType;
		using point_type = child_type;
		virtual DType distance_raw(const child_type& point) const = 0; // avoid (square) root operations to speed up
		virtual DType distance(const child_type& point) const = 0;
		virtual child_type centroid(const std::vector<child_type>& points_ptr) const = 0;
		
		virtual child_type operator+(const child_type& point) const = 0;
		virtual child_type operator*(float factor) const = 0;
		virtual child_type operator/(float factor) const = 0;
		
		static child_type centroid_s(const std::vector<child_type>& points)
		{
			child_type data;
			data = data.centroid(points);
			return data;
		}
	};
	
	template <typename data_point>
	class cluster
	{
	public:
		std::vector<data_point> points;
		
		data_point centroid()
		{
			data_point output;
			output = output.centroid(points);
			return output;
		}
	};
	
	class hierarchical_clustering
	{
	public:
		explicit hierarchical_clustering(bool _verbose = false, int _stop_cluster_size = 0, float _stop_majority_ratio = 1.0) : stop_cluster_size(_stop_cluster_size), stop_majority_ratio(_stop_majority_ratio), verbose(_verbose)
		{
		
		}
		
		bool verbose;
		int stop_cluster_size;
		float stop_majority_ratio;
		
		template <typename child_type>
		class single_cluster
		{
		public:
			single_cluster(size_t _index, size_t _merged_from_1, size_t _merged_from_2, size_t size_1, size_t size_2, child_type sum_1, child_type sum_2, const std::vector<size_t>& ele1, const std::vector<size_t>& ele2)
			{
				index = _index;
				merged_from_1 = _merged_from_1;
				merged_from_2 = _merged_from_2;
				size = size_1 + size_2;
				sum = sum_1 + sum_2;
				elements.insert(elements.end(), ele1.begin(), ele1.end());
				elements.insert(elements.end(), ele2.begin(), ele2.end());
			}
			std::vector<size_t> elements;
			size_t index;
			size_t merged_from_1;
			size_t merged_from_2;
			size_t size;
			child_type sum;
			child_type centroid()
			{
				return sum / size;
			}
		};
		
		class cluster_merge_summary
		{
		public:
			enum class data_type
			{
				point, cluster
			};
			
			[[nodiscard]] std::stringstream print() const
			{
				std::stringstream output;
				for (auto&& operation: merge_operations)
				{
					auto ss = operation.print();
					output << ss.str() << std::endl;
				}
				return output;
			}
			
			static std::stringstream print_all_summary(const std::vector<cluster_merge_summary>& summary)
			{
				std::stringstream output;
				for (int i = 0; i < summary.size(); ++i)
				{
					output << "iteration " << i << std::endl;
					auto ss = summary[i].print();
					output << ss.str();
				}
				return output;
			}
			
			class merge_operation
			{
			public:
				merge_operation(data_type _type1, data_type _type2, size_t _index1, size_t _index2, size_t _size1, size_t _size2, size_t _index_after, const std::vector<size_t>& ele1, const std::vector<size_t>& ele2)
				{
					type1 = _type1;
					type2 = _type2;
					index1 = _index1;
					index2= _index2;
					size1 = _size1;
					size2 = _size2;
					size_after_merge = _size1 + _size2;
					index_after = _index_after;
					elements_index.insert(elements_index.end(), ele1.begin(), ele1.end());
					elements_index.insert(elements_index.end(), ele2.begin(), ele2.end());
				}
				data_type type1;
				data_type type2;
				size_t index1;
				size_t index2;
				size_t size1;
				size_t size2;
				size_t size_after_merge;
				size_t index_after;
				std::vector<size_t> elements_index;
				
				[[nodiscard]] std::stringstream print() const
				{
					std::stringstream ss;
					ss << "merge ";
					if (type1 == data_type::point) ss << "point " << index1 << "(" << size1 << ")";
					if (type1 == data_type::cluster) ss << "cluster " << index1 << "(" << size1 << ")";
					ss << " with ";
					if (type2 == data_type::point) ss << "point " << index2 << "(" << size2 << ")";
					if (type2 == data_type::cluster) ss << "cluster " << index2 << "(" << size2 << ")";
					ss << " to cluster " << index_after << "(" << size_after_merge << ")";
					return ss;
				}
			};
			
			size_t iteration;
			
			std::vector<merge_operation> merge_operations;
		};
		
		template <typename child_type>
		std::vector<cluster_merge_summary> process(const std::vector<child_type>& data_points, int link_per_iteration = 1)
		{
			using DType = typename child_type::data_type;
			const auto INF = std::numeric_limits<DType>::max();
			size_t data_size = data_points.size();
			int iter = 0;
			
			std::vector<single_cluster<child_type>> cluster_container;
			cluster_container.reserve(data_size);
			std::vector<cluster_merge_summary> cluster_state_history;
			
			const size_t distances_array_side_length = data_size * 2;
			const size_t distances_array_total_length = distances_array_side_length * distances_array_side_length;
			std::vector<bool> alive;
			alive.resize(distances_array_side_length);
			for (int i = 0; i < data_size; ++i)
			{
				alive[i] = true;
			}
			for (int i = 0; i < data_size; ++i)
			{
				alive[data_size + i] = false;
			}

			auto* distances = new DType[distances_array_total_length];
			for (int i = 0; i < distances_array_total_length; ++i) distances[i] = INF;
			
			auto distance_2d_array = [&distances, distances_array_side_length](size_t x, size_t y) -> DType& {
				return distances[y*distances_array_side_length + x];
			};
			auto get_distance = [&distances, distances_array_side_length](size_t x, size_t y) -> DType {
				if (y > x) std::swap(x,y); //always x > y
				return distances[y*distances_array_side_length + x];
			};
			auto get_size_on_distance_map = [data_size, &cluster_container](size_t x) -> size_t {
				if (x < data_size)
				{
					return 1;
				}
				else
				{
					return cluster_container[x - data_size].size;
				}
			};
			auto get_centroid = [data_size, &data_points, &cluster_container](size_t x) -> child_type {
				if (x < data_size)
				{
					return data_points[x];
				}
				else
				{
					return cluster_container[x-data_size].centroid();
				}
			};
			auto clear_distance = [&distances, distances_array_side_length, INF](size_t x){
				for (int i = 0; i < distances_array_side_length; ++i)
				{
					distances[i*distances_array_side_length + x] = INF;
					distances[x*distances_array_side_length + i] = INF;
				}
			};
			auto get_name = [data_size](size_t x) -> std::string {
				if (x < data_size) return "point " + std::to_string(x);
				else return "cluster " + std::to_string(x-data_size);
			};
			auto print_distance_map = [INF, distance_2d_array, distances_array_side_length](){
				std::cout << "distance_2d_array start" << std::endl;
				for (int y = 0; y < distances_array_side_length; ++y)
				{
					for (int x = 0; x < distances_array_side_length; ++x)
					{
						if (distance_2d_array(x,y) == INF)
						{
							std::cout << "INF" << "\t";
						}
						else
						{
							std::cout << std::setprecision(3) << distance_2d_array(x,y) << "\t";
						}
					}
					std::cout << std::endl;
				}
				std::cout << "distance_2d_array stop" << std::endl;
			};
			
			for (int y = 0; y < data_size; ++y)
			{
				for (int x = y + 1; x < data_size; ++x)
				{
					distance_2d_array(x,y) = data_points[x].distance(data_points[y]);
				}
			}
			
			std::vector<size_t> link_index_1;link_index_1.resize(link_per_iteration);
			std::vector<size_t> link_index_2;link_index_2.resize(link_per_iteration);
			std::vector<DType> link_distance;link_distance.resize(link_per_iteration);
			while (true)
			{
				iter++;
				if (verbose) std::cout << "iteration: " << std::to_string(iter) << std::endl;
				
				bool exit = false;
				
				//find the minimal distance
				for (int i = 0; i < link_per_iteration; ++i)
				{
					link_index_1[i] = 0;
					link_index_2[i] = 0;
					link_distance[i] = INF;
				}
				for (int y = 0; y < distances_array_side_length; ++y)
				{
					for (int x = y + 1; x < distances_array_side_length; ++x)
					{
						auto distance = get_distance(x, y);
						if (distance < link_distance[0])
						{
							link_distance[0] = distance;
							link_index_1[0] = x;
							link_index_2[0] = y;
							
							//repair the link_distance
							for (int i = 0; i < link_per_iteration - 1; ++i)
							{
								if (link_distance[i] < link_distance[i+1])
								{
									std::swap(link_distance[i], link_distance[i+1]);
									std::swap(link_index_1[i],link_index_1[i+1]);
									std::swap(link_index_2[i],link_index_2[i+1]);
								}
							}
						}
					}
				}
				
				//merge cluster
				std::set<size_t> merged_index;
				for (int i = link_per_iteration-1; i >=0 ; --i)
				{
					if ((merged_index.find(link_index_1[i]) == merged_index.end()) && (merged_index.find(link_index_2[i]) == merged_index.end()))
					{
						merged_index.insert(link_index_1[i]);
						merged_index.insert(link_index_2[i]);
					}
					else
					{
						//duplicate
						link_index_1[i] = 0;
						link_index_2[i] = 0;
						link_distance[i] = INF;
					}
				}
				
				cluster_merge_summary summary;
				summary.iteration = iter;
				for (int i = 0; i < link_per_iteration; ++i)
				{
					if (link_index_1[i] == 0 && link_index_2[i] == 0 && link_distance[i] == INF) continue;
					size_t size1 = get_size_on_distance_map(link_index_1[i]);
					size_t size2 = get_size_on_distance_map(link_index_2[i]);
					child_type sum1 = get_centroid(link_index_1[i]);
					child_type sum2 = get_centroid(link_index_2[i]);
					std::vector<size_t> ele1, ele2;
					if (link_index_1[i] < data_size) ele1 = {link_index_1[i]};
					else ele1 = cluster_container[link_index_1[i] - data_size].elements;
					if (link_index_2[i] < data_size) ele2 = {link_index_2[i]};
					else ele2 = cluster_container[link_index_2[i] - data_size].elements;
					single_cluster new_cluster(cluster_container.size() + data_size, link_index_1[i], link_index_2[i], size1, size2, sum1, sum2, ele1, ele2);
					
					alive[new_cluster.index] = true;
					cluster_container.push_back(new_cluster);
					
					cluster_merge_summary::data_type type1, type2;
					size_t index1, index2;
					if (link_index_1[i] < data_size)
					{
						type1 = cluster_merge_summary::data_type::point;
						index1 = link_index_1[i];
					}
					else
					{
						type1 = cluster_merge_summary::data_type::cluster;
						index1 = link_index_1[i] - data_size;
					}
					if (link_index_2[i] < data_size)
					{
						type2 = cluster_merge_summary::data_type::point;
						index2 = link_index_2[i];
					}
					else
					{
						type2 = cluster_merge_summary::data_type::cluster;
						index2 = link_index_2[i] - data_size;
					}
					summary.merge_operations.emplace_back(type1, type2, index1, index2, size1, size2, new_cluster.index-data_size, ele1, ele2);
					
					if (verbose)
					{
						std::cout << "merge " << get_name(link_index_1[i]) << "(" << std::to_string(size1) << ")" << " with " << get_name(link_index_2[i]) << "(" << std::to_string(size2) << ")"  << " to " << get_name(new_cluster.index) << ", dis: " << link_distance[i] << std::endl;
					}
					
					//check majority ratio
					if (new_cluster.size / float(data_size) > stop_majority_ratio) exit = true;
					
					//calculate new distances
					size_t loc_in_distance_map = new_cluster.index;
					for (int y = 0; y < distances_array_side_length; ++y)
					{
						if (y == loc_in_distance_map)
						{
							distance_2d_array(loc_in_distance_map, y) = INF;
							continue;
						}
						if (!alive[y])
						{
							distance_2d_array(loc_in_distance_map, y) = INF;
							continue;
						}
						auto new_cluster_centroid = new_cluster.centroid();
						if (y < data_size)
						{
							//single data
							distance_2d_array(loc_in_distance_map, y) = data_points[y].distance(new_cluster_centroid);
						}
						else
						{
							//cluster
							distance_2d_array(loc_in_distance_map, y) = cluster_container[y - data_size].centroid().distance(new_cluster_centroid);
						}
					}
				}
				cluster_state_history.push_back(summary);
				
				//clear merged cluster
				for (int i = 0; i < link_per_iteration; ++i)
				{
					if (link_index_1[i] == 0 && link_index_2[i] == 0 && link_distance[i] == INF) continue;
					alive[link_index_1[i]] = false;
					alive[link_index_2[i]] = false;
					clear_distance(link_index_1[i]);
					clear_distance(link_index_2[i]);
				}
				
				////print distance map
				//print_distance_map();
				
				//check cluster size
				int counter_cluster = 0, counter_point = 0;
				for (size_t i = data_size; i < data_size*2; ++i)
				{
					if (alive[i]) counter_cluster++;
				}
				for (size_t i = 0; i < data_size; ++i)
				{
					if (alive[i]) counter_point++;
				}
				if (counter_point + counter_cluster <= stop_cluster_size ) exit = true;
				if (counter_point + counter_cluster == 1 ) exit = true;
				if (exit) break;
			}
			
			delete[] distances;
			
			return cluster_state_history;
		}
	
	};
	
	//Sample implementation below
	template <typename DType>
	class data_point_1d : public data_point<DType, data_point_1d<DType>>
	{
	public:
		using data_type = DType;
		data_point_1d(DType _x = 0) : x(_x) {};
		
		DType x;
		DType distance_raw(const data_point_1d<DType>& point) const override
		{
			DType x_diff = point.x - x;
			if (x_diff < 0) x_diff = -x_diff;
			return x_diff;
		}
		
		DType distance(const data_point_1d<DType>& point) const override
		{
			return distance_raw(point);
		}
		
		data_point_1d centroid(const std::vector<data_point_1d>& points) const override
		{
			DType x_sum = 0;
			auto size = points.size();
			auto points_ptr = points.data();
			for (int i = 0; i < size; ++i)
			{
				x_sum += points_ptr[i].x;
			}
			return data_point_1d(x_sum/size);
		}
		
		data_point_1d operator+(const data_point_1d& point) const override
		{
			return data_point_1d(x + point.x);
		}
		
		data_point_1d operator*(float factor) const override
		{
			return data_point_1d(x * factor);
		}
		
		data_point_1d operator/(float factor) const override
		{
			return data_point_1d(x / factor);
		}
	};
	
	template <typename DType>
	class data_point_2d : public data_point<DType, data_point_2d<DType>>
	{
	public:
		using data_type = DType;
		data_point_2d(DType _x = 0, DType _y = 0) : x(_x), y(_y) {};
		
		DType x;
		DType y;
		DType distance_raw(const data_point_2d<DType>& point) const override
		{
			DType x_diff = point.x - x;
			DType y_diff = point.y - y;
			return x_diff * x_diff + y_diff * y_diff;
		}
		
		DType distance(const data_point_2d<DType>& point) const override
		{
			return sqrt(distance_raw(point));
		}
		
		data_point_2d centroid(const std::vector<data_point_2d>& points) const override
		{
			DType x_sum = 0;
			DType y_sum = 0;
			auto size = points.size();
			auto points_ptr = points.data();
			for (int i = 0; i < points.size(); ++i)
			{
				x_sum += points_ptr[i].x;
				y_sum += points_ptr[i].y;
			}
			return data_point_2d(x_sum/size, y_sum/size);
		}
		
		data_point_2d operator+(const data_point_2d& point) const override
		{
			return data_point_2d(x + point.x, y + point.y);
		}
		
		data_point_2d operator*(float factor) const override
		{
			return data_point_2d(x * factor, y * factor);
		}
		
		data_point_2d operator/(float factor) const override
		{
			return data_point_2d(x / factor, y / factor);
		}

	};
	
}

