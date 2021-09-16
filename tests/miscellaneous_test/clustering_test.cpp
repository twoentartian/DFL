#include <string>
#include <iostream>

#include <clustering/hierarchical_clustering.hpp>

int main()
{
	
	std::vector<clustering::data_point_2d<float>> points = {{0.0,0.1}, {0.0,0.2}, {10.0,0.1},{10.0,0.1}};
	
	clustering::hierarchical_clustering temp(false, 1, 0.5);
	auto history = temp.process(points, 2);
	std::cout << clustering::hierarchical_clustering::cluster_merge_summary::print_all_summary(history).str() << std::endl;
	
}