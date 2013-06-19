// Emacs Mode Line: -*- Mode:c++;-*-
// -------------------------------------------------------------
/**
 * @file   graph_partioner_implementation.hpp
 * @author William A. Perkins
 * @date   2013-06-19 11:29:52 d3g096
 * 
 * @brief  
 * 
 * 
 */
// -------------------------------------------------------------


#ifndef _graph_partioner_implementation_hpp_
#define _graph_partioner_implementation_hpp_

#include <gridpack/partition/adjacency_list.hpp>

namespace gridpack {
namespace network {

// -------------------------------------------------------------
//  class GraphPartitionerImplementation
// -------------------------------------------------------------
class GraphPartitionerImplementation 
  : public parallel::Distributed,
    private utility::Uncopyable
{
public:

  /// The index type
  typedef AdjacencyList::Index Index;

  /// A vector of Indexes
  typedef AdjacencyList::IndexVector IndexVector;

  /// Default constructor.
  GraphPartitionerImplementation(const parallel::Communicator& comm);

  /// Construct w/ known local sizes  (guesses to size containers, maybe)
  GraphPartitionerImplementation(const parallel::Communicator& comm,
                                 const int& local_nodes, const int& local_edges);

  /// Destructor
  virtual ~GraphPartitionerImplementation(void);

  /// Add the global index of a local node
  void add_node(const Index& node_index)
  {
    p_adjacency_list.add_node(node_index);
  }
  
  /// Add the global index of a local edge and what it connects
  void add_edge(const Index& edge_index, 
                const Index& node_index_1,
                const Index& node_index_2)
  {
    p_adjacency_list.add_edge(edge_index, node_index_1, node_index_2);
  }

  /// Get the number of local nodes
  size_t nodes(void) const
  {
    return p_adjacency_list.nodes();
  }

  /// Get the global node index given a local index
  Index node_index(const int& local_index) const
  {
    return p_adjacency_list.node_index(local_index);
  }

  /// Get the number of local edges
  size_t edges(void) const
  {
    return p_adjacency_list.edges();
  }

  /// Get the global edge index given a local index
  Index edge_index(const int& local_index) const
  {
    return p_adjacency_list.edge_index(local_index);
  }

  /// Partition the graph
  void partition(void)
  {
    p_adjacency_list.ready();
    this->p_partition();
  }

  /// Get the node destinations
  void node_destinations(IndexVector& dest) const;

  /// Get the edge destinations
  void edge_destinations(IndexVector& dest) const;

protected:

  /// Adjacency list builder
  AdjacencyList p_adjacency_list;

  /// A list of processors where local nodes should go
  IndexVector p_node_destinations;

  /// A list of processors where local edges should go
  IndexVector p_edge_destinations;

  /// Partition the graph (specialized)
  virtual void p_partition(void) = 0;
};


} // namespace network
} // namespace gridpack


#endif
