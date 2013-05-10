/*------------------------------------------------------------------------*/
/*                 Copyright 2010 Sandia Corporation.                     */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*  Export of this program may require a license from the                 */
/*  United States Government.                                             */
/*------------------------------------------------------------------------*/

#include <iostream>
#include <sstream>

#include <stk_util/unit_test_support/stk_utest_macros.hpp>

#include <stk_util/parallel/Parallel.hpp>

#include <stk_mesh/base/BulkData.hpp>
#include <stk_mesh/base/GetEntities.hpp>
#include <stk_mesh/base/Comm.hpp>

#include <stk_mesh/fixtures/RingFixture.hpp>

#include <unit_tests/UnitTestModificationEndWrapper.hpp>

using stk::mesh::Part;
using stk::mesh::MetaData;
using stk::mesh::BulkData;
using stk::mesh::Entity;
using stk::mesh::Selector;
using stk::mesh::PartVector;
using stk::mesh::fixtures::RingFixture;

//----------------------------------------------------------------------

STKUNIT_UNIT_TEST(UnitTestingOfBulkData, testChangeParts)
{
  // This unit test tests part operations and verifies operations
  // by looking at bucket supersets. We use contrived entities
  // (as opposed to a fixture) for simplicity and clarity.

  stk::ParallelMachine pm = MPI_COMM_WORLD;
  MPI_Barrier( pm );

  const int p_size = stk::parallel_machine_size( pm );
  const int p_rank = stk::parallel_machine_rank( pm );

  // Single process, no sharing

  // Meta data with entity ranks [0..3]
  const unsigned spatial_dimension = 3;
  std::vector<std::string> entity_names(spatial_dimension+2); // extra rank name (simulate something like constraint)
  for ( size_t i = 0 ; i < spatial_dimension+2 ; ++i ) {
    std::ostringstream name ;
    name << "EntityRank_" << i ;
    entity_names[i] = name.str();
  }

  // Create a mesh with a bunch of parts

  MetaData meta( spatial_dimension, entity_names );
  BulkData bulk( meta , pm , 100 );

  Part & part_univ   = meta.universal_part();
  Part & part_owns   = meta.locally_owned_part();
  Part & part_shared = meta.globally_shared_part();

  Part & part_A_0 = meta.declare_part(std::string("A_0"), 0 /*entity_rank*/);
  Part & part_A_1 = meta.declare_part(std::string("A_1"), 1 /*entity_rank*/);
  Part & part_A_2 = meta.declare_part(std::string("A_2"), 2 /*entity_rank*/);
  Part & part_A_3 = meta.declare_part(std::string("A_3"), 3 /*entity_rank*/);

  Part & part_B_0 = meta.declare_part(std::string("B_0"), 0 /*entity_rank*/);
  Part & part_B_2 = meta.declare_part(std::string("B_2"), 2 /*entity_rank*/);

  meta.commit();
  bulk.modification_begin();

  PartVector tmp(1), no_parts;

  // Declare a few entities of various ranks. In order for the sharing
  // to work, we need to have all the entities we'll be playing with
  // to be in the owned-closure of a high-level non-shared entity, we'll
  // call that entity the closure_entity because all the other entities
  // will be in it's closure.

  Entity closure_entity = bulk.declare_entity(4 /*entity rank*/,
                                               p_rank+1 /*id*/,
                                               no_parts);

  tmp[0] = & part_A_0 ;
  Entity entity_0_1 = bulk.declare_entity(MetaData::NODE_RANK, 1 /*id*/, tmp);
  bulk.declare_relation( closure_entity , entity_0_1 , 0 /*local_rel_id*/ );

  tmp[0] = & part_A_1 ;
  Entity entity_1_1 = bulk.declare_entity(MetaData::EDGE_RANK, 1 /*id*/, tmp);
  bulk.declare_relation( closure_entity , entity_1_1 , 1 /*local_rel_id*/ );

  tmp[0] = & part_A_2 ;
  Entity entity_2_1 = bulk.declare_entity(MetaData::FACE_RANK, 1 /*id*/, tmp);
  bulk.declare_relation( closure_entity , entity_2_1 , 2 /*local_rel_id*/ );

  tmp[0] = & part_A_3 ;
  Entity entity_3_1 = bulk.declare_entity(MetaData::ELEMENT_RANK, 1 /*id*/, tmp);
  bulk.declare_relation( closure_entity , entity_3_1 , 3 /*local_rel_id*/ );

  // Ensure that the supersets of the buckets containing the entities we
  // just created are correct.

  bulk.bucket(entity_0_1).supersets( tmp );
  STKUNIT_ASSERT_EQUAL( size_t(3) , tmp.size() );
  STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_univ) );
  STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_owns) );
  STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_A_0) );

  bulk.bucket(entity_1_1).supersets( tmp );
  STKUNIT_ASSERT_EQUAL( size_t(3) , tmp.size() );
  STKUNIT_ASSERT( bulk.bucket(entity_1_1).member(part_univ) );
  STKUNIT_ASSERT( bulk.bucket(entity_1_1).member(part_owns) );
  STKUNIT_ASSERT( bulk.bucket(entity_1_1).member(part_A_1) );

  bulk.bucket(entity_2_1).supersets( tmp );
  STKUNIT_ASSERT_EQUAL( size_t(3) , tmp.size() );
  STKUNIT_ASSERT( bulk.bucket(entity_2_1).member(part_univ) );
  STKUNIT_ASSERT( bulk.bucket(entity_2_1).member(part_owns) );
  STKUNIT_ASSERT( bulk.bucket(entity_2_1).member(part_A_2) );

  bulk.bucket(entity_3_1).supersets( tmp );
  STKUNIT_ASSERT_EQUAL( size_t(3) , tmp.size() );
  STKUNIT_ASSERT( bulk.bucket(entity_3_1).member(part_univ) );
  STKUNIT_ASSERT( bulk.bucket(entity_3_1).member(part_owns) );
  STKUNIT_ASSERT( bulk.bucket(entity_3_1).member(part_A_3) );

  // Add entity_0_1 to the part it was already in
  {
    tmp.resize(1);
    tmp[0] = & part_A_0 ;
    bulk.change_entity_parts( entity_0_1 , tmp );
    bulk.bucket(entity_0_1).supersets( tmp );
    STKUNIT_ASSERT_EQUAL( size_t(3) , tmp.size() );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_univ) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_owns) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_A_0) );
  }

  // Add entity_0_1 to part_B_0
  {
    tmp.resize(1);
    tmp[0] = & part_B_0 ;
    bulk.change_entity_parts( entity_0_1 , tmp );
    bulk.bucket(entity_0_1).supersets( tmp );
    STKUNIT_ASSERT_EQUAL( size_t(4) , tmp.size() );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_univ) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_owns) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_A_0) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_B_0) );
  }

  // Remove entity_0_1 from the part it was just added to above
  {
    tmp.resize(1);
    tmp[0] = & part_B_0 ;
    bulk.change_entity_parts( entity_0_1 , PartVector() , tmp );
    bulk.bucket(entity_0_1).supersets( tmp );
    STKUNIT_ASSERT_EQUAL( size_t(3) , tmp.size() );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_univ) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_owns) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_A_0) );
  }

  // Add relation from entity_1_1 (which is in part_A_1) to
  // entity_0_1 (which is in part_A_0). After the relation
  // is added, there is an induced membership of entity_0_1
  // within part A_1.
  stk::mesh::RelationIdentifier test_rel_id = 0;
  {
    bulk.declare_relation( entity_1_1 , entity_0_1 , test_rel_id );
    bulk.bucket(entity_0_1).supersets( tmp );
    STKUNIT_ASSERT_EQUAL( size_t(4) , tmp.size() );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_univ) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_owns) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_A_0) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_A_1) );
  }

  // Remove the relationship added in the step above and
  // demonstrate that the induced membership of entity_0_1
  // in part_A_1 is gone
  {
    bulk.destroy_relation( entity_1_1 , entity_0_1, test_rel_id );
    bulk.bucket(entity_0_1).supersets( tmp );
    STKUNIT_ASSERT_EQUAL( size_t(3) , tmp.size() );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_univ) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_owns) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_A_0) );
  }

  // Add entity_2_1 to part_B_2
  {
    tmp.resize(1);
    tmp[0] = & part_B_2 ;
    bulk.change_entity_parts( entity_2_1 , tmp );
    bulk.bucket(entity_2_1).supersets( tmp );
    STKUNIT_ASSERT_EQUAL( size_t(4) , tmp.size() );
    STKUNIT_ASSERT( bulk.bucket(entity_2_1).member(part_univ) );
    STKUNIT_ASSERT( bulk.bucket(entity_2_1).member(part_owns) );
    STKUNIT_ASSERT( bulk.bucket(entity_2_1).member(part_A_2) );
    STKUNIT_ASSERT( bulk.bucket(entity_2_1).member(part_B_2) );
  }

  // Add relation from entity_2_1 (which is in part_A_2 and B_2) to
  // entity_0_1 (which is in part_A_0). After the relation
  // is added, there is an induced membership of entity_0_1
  // within entity_2_1's parts (A_2 and B_2) (and of course entity_0_1
  // is still in the parts it was already in).
  {
    bulk.declare_relation( entity_2_1 , entity_0_1 , test_rel_id );
    bulk.bucket(entity_0_1).supersets( tmp );
    STKUNIT_ASSERT_EQUAL( size_t(5) , tmp.size() );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_univ) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_owns) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_A_0) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_A_2) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_B_2) );
  }

  // Remove the relationship added in the step above and
  // demonstrate that the induced membership of entity_0_1
  // in parts A_2 and B_2 is gone.
  {
    bulk.destroy_relation( entity_2_1 , entity_0_1, test_rel_id );
    bulk.bucket(entity_0_1).supersets( tmp );
    STKUNIT_ASSERT_EQUAL( size_t(3) , tmp.size() );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_univ) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_owns) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_A_0) );
  }

  bulk.modification_end();

  //------------------------------
  // Now the parallel fun.  Existing entities should be shared
  // by all processes since they have the same identifiers.
  // They should also have the same parts.

  bool parallel = p_size > 1;

  // For parallel runs, the entities should be in the same parts
  // as they were before the modification end and they should
  // be in the shared part as well.

  bulk.bucket(entity_0_1).supersets( tmp );
  if ( bulk.parallel_owner_rank(entity_0_1) == p_rank ) {
    STKUNIT_ASSERT_EQUAL( size_t(parallel ? 4 : 3) , tmp.size() );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_univ) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_owns) );
    if ( parallel )
      STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_shared) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_A_0) );
  }
  else {
    STKUNIT_ASSERT( parallel );
    STKUNIT_ASSERT_EQUAL( size_t(3) , tmp.size() );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_univ) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_shared) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_A_0) );
  }

  bulk.bucket(entity_2_1).supersets( tmp );
  if ( bulk.parallel_owner_rank(entity_2_1) == p_rank ) {
    STKUNIT_ASSERT_EQUAL( size_t(parallel ? 5 : 4) , tmp.size() );
    STKUNIT_ASSERT( bulk.bucket(entity_2_1).member(part_univ) );
    STKUNIT_ASSERT( bulk.bucket(entity_2_1).member(part_owns) );
    if ( parallel )
      STKUNIT_ASSERT( bulk.bucket(entity_2_1).member(part_shared) );
    STKUNIT_ASSERT( bulk.bucket(entity_2_1).member(part_A_2) );
    STKUNIT_ASSERT( bulk.bucket(entity_2_1).member(part_B_2) );
  }
  else {
    STKUNIT_ASSERT_EQUAL( size_t(4) , tmp.size() );
    STKUNIT_ASSERT( bulk.bucket(entity_2_1).member(part_univ) );
    STKUNIT_ASSERT( bulk.bucket(entity_2_1).member(part_shared) );
    STKUNIT_ASSERT( bulk.bucket(entity_2_1).member(part_A_2) );
    STKUNIT_ASSERT( bulk.bucket(entity_2_1).member(part_B_2) );
  }

  if ( parallel ) {
    // If parallel, check that the entities are shared across all procs.
    STKUNIT_ASSERT_EQUAL( size_t(p_size - 1) , bulk.entity_comm_sharing(bulk.entity_key(entity_0_1)).size() );
    STKUNIT_ASSERT_EQUAL( size_t(p_size - 1) , bulk.entity_comm_sharing(bulk.entity_key(entity_1_1)).size() );
    STKUNIT_ASSERT_EQUAL( size_t(p_size - 1) , bulk.entity_comm_sharing(bulk.entity_key(entity_2_1)).size() );
    STKUNIT_ASSERT_EQUAL( size_t(p_size - 1) , bulk.entity_comm_sharing(bulk.entity_key(entity_3_1)).size() );
  }

  bulk.modification_begin();

  // Add entity_0_1 to a new part on the owning process

  int ok_to_modify = bulk.parallel_owner_rank(entity_0_1) == p_rank ;

  try {
    tmp.resize(1);
    tmp[0] = & part_B_0 ;
    bulk.change_entity_parts( entity_0_1 , tmp );
    STKUNIT_ASSERT( ok_to_modify );
  }
  catch( const std::exception & x ) {
    STKUNIT_ASSERT( ! ok_to_modify );
  }

  // Check that entity_0_1 is in the new part on the owning
  // process, but not on other processes.

  bulk.bucket(entity_0_1).supersets( tmp );
  if ( bulk.parallel_owner_rank(entity_0_1) == p_rank ) {
    STKUNIT_ASSERT_EQUAL( size_t(parallel ? 5 : 4) , tmp.size() );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_univ) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_owns) );
    if ( parallel )
      STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_shared) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_A_0) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_B_0) );
  }
  else {
    STKUNIT_ASSERT_EQUAL( size_t(3) , tmp.size() );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_univ) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_shared) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_A_0) );
  }

  bulk.modification_end();

  // Now that modification_end has been called, entity_0_1 should
  // be in the new part (B_0) on all processes.

  bulk.bucket(entity_0_1).supersets( tmp );
  if ( bulk.parallel_owner_rank(entity_0_1) == p_rank ) {
    STKUNIT_ASSERT_EQUAL( size_t(parallel ? 5 : 4) , tmp.size() );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_univ) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_owns) );
    if ( parallel )
      STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_shared) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_A_0) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_B_0) );
  }
  else {
    STKUNIT_ASSERT_EQUAL( size_t(4) , tmp.size() );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_univ) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_shared) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_A_0) );
    STKUNIT_ASSERT( bulk.bucket(entity_0_1).member(part_B_0) );
  }
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------

STKUNIT_UNIT_TEST(UnitTestingOfBulkData, testChangeParts_ringmesh)
{
  // This unit test tests part operations and verifies operations
  // by looking at bucket supersets. We use RingMesh for a slightly
  // more realistic test than the test above but it's a bit harder
  // to read.

  stk::ParallelMachine pm = MPI_COMM_WORLD;
  MPI_Barrier( pm );

  const unsigned nPerProc   = 10;
  const int p_rank     = stk::parallel_machine_rank( pm );
  const int p_size     = stk::parallel_machine_size( pm );
  const unsigned nLocalNode = nPerProc + ( 1 < p_size ? 1 : 0 );
  const unsigned nLocalElement = nPerProc ;

  // Create the ring mesh

  RingFixture ring_mesh( pm , nPerProc , true /* generate parts */ );
  ring_mesh.m_meta_data.commit();
  BulkData& bulk = ring_mesh.m_bulk_data;

  bulk.modification_begin();
  ring_mesh.generate_mesh( );
  STKUNIT_ASSERT(stk::unit_test::modification_end_wrapper(bulk,
                                                          false /* no aura */));

  bulk.modification_begin();
  ring_mesh.fixup_node_ownership();
  STKUNIT_ASSERT(stk::unit_test::modification_end_wrapper(bulk,
                                                          false /* no aura */));

  Part & part_owns = ring_mesh.m_meta_data.locally_owned_part();
  Part & part_univ = ring_mesh.m_meta_data.universal_part();

  // Check that local elements are in the expected parts. Note that the
  // RingMesh puts each element in its own part.
  for ( unsigned i = 0 ; i < nLocalElement ; ++i ) {
    const unsigned n = i + nPerProc * p_rank ;
    Entity const element = bulk.get_entity( MetaData::ELEMENT_RANK /*entity rank*/,
                                              ring_mesh.m_element_ids[n] );
    STKUNIT_ASSERT( bulk.is_valid(element) );
    STKUNIT_ASSERT( bulk.bucket(element).member( part_univ ) );
    STKUNIT_ASSERT( bulk.bucket(element).member( part_owns ) );
    STKUNIT_ASSERT( bulk.bucket(element).member( * ring_mesh.m_element_parts[ n % ring_mesh.m_element_parts.size() ] ) );
  }

  // Check that local nodes are in the expected parts. Note that the relations
  // that nodes have to elements should cause induced membership of the node
  // in the parts of both elements it touches.
  for ( unsigned i = 0 ; i < nLocalNode ; ++i ) {
    const unsigned n = ( i + nPerProc * p_rank ) % ring_mesh.m_node_ids.size();
    const unsigned e0 = n ;
    const unsigned e1 = ( n + ring_mesh.m_element_ids.size() - 1 ) % ring_mesh.m_element_ids.size();
    const unsigned ns = ring_mesh.m_element_parts.size();
    const unsigned n0 = e0 % ns ;
    const unsigned n1 = e1 % ns ;
    Part * const epart_0 = ring_mesh.m_element_parts[ n0 < n1 ? n0 : n1 ];
    Part * const epart_1 = ring_mesh.m_element_parts[ n0 < n1 ? n1 : n0 ];

    Entity const node = bulk.get_entity( MetaData::NODE_RANK , ring_mesh.m_node_ids[n] );
    STKUNIT_ASSERT( bulk.is_valid(node) );
    if ( bulk.parallel_owner_rank(node) == p_rank ) {
      STKUNIT_ASSERT( bulk.bucket(node).member( part_univ ) );
      STKUNIT_ASSERT( bulk.bucket(node).member( part_owns ) );
      STKUNIT_ASSERT( bulk.bucket(node).member( *epart_0 ) );
      STKUNIT_ASSERT( bulk.bucket(node).member( *epart_1 ) );
    }
    else {
      STKUNIT_ASSERT( bulk.bucket(node).member( part_univ ) );
      STKUNIT_ASSERT( ! bulk.bucket(node).member( part_owns ) );
      STKUNIT_ASSERT( bulk.bucket(node).member( * epart_0 ) );
      STKUNIT_ASSERT( bulk.bucket(node).member( * epart_1 ) );
    }
  }

  bulk.modification_begin();

  // On rank 0, change all locally owned elements to the extra-part then check
  // for correct part membership
  if ( 0 == p_rank ) {
    for ( unsigned i = 0 ; i < nLocalElement ; ++i ) {
      const unsigned n = i + nPerProc * p_rank ;

      PartVector add(1); add[0] = & ring_mesh.m_element_part_extra ;
      PartVector rem(1); rem[0] = ring_mesh.m_element_parts[ n % ring_mesh.m_element_parts.size() ];

      Entity const element = bulk.get_entity( MetaData::ELEMENT_RANK , ring_mesh.m_element_ids[n] );
      bulk.change_entity_parts( element , add , rem );
      STKUNIT_ASSERT( bulk.bucket(element).member( part_univ ) );
      STKUNIT_ASSERT( bulk.bucket(element).member( part_owns ) );
      STKUNIT_ASSERT( bulk.bucket(element).member(ring_mesh.m_element_part_extra ) );
    }
  }

  bulk.modification_end();

  // Modification end has been called, check that the part changes made
  // in the previous step are reflected across the other procs.
  for ( unsigned i = 0 ; i < nLocalNode ; ++i ) {
    const unsigned n = ( i + nPerProc * p_rank ) % ring_mesh.m_node_ids.size();
    const unsigned e0 = n ;
    const unsigned e1 = ( n + ring_mesh.m_element_ids.size() - 1 ) % ring_mesh.m_element_ids.size();
    const unsigned ns = ring_mesh.m_element_parts.size();
    const unsigned n0 = e0 % ns ;
    const unsigned n1 = e1 % ns ;
    Part * ep_0 = e0 < nLocalElement ? & ring_mesh.m_element_part_extra : ring_mesh.m_element_parts[n0] ;
    Part * ep_1 = e1 < nLocalElement ? & ring_mesh.m_element_part_extra : ring_mesh.m_element_parts[n1] ;

    Part * epart_0 = ep_0->mesh_meta_data_ordinal() < ep_1->mesh_meta_data_ordinal() ? ep_0 : ep_1 ;
    Part * epart_1 = ep_0->mesh_meta_data_ordinal() < ep_1->mesh_meta_data_ordinal() ? ep_1 : ep_0 ;

    Entity const node = bulk.get_entity( MetaData::NODE_RANK , ring_mesh.m_node_ids[n] );
    STKUNIT_ASSERT( bulk.is_valid(node) );
    if ( bulk.parallel_owner_rank(node) == p_rank ) {
      STKUNIT_ASSERT( bulk.bucket(node).member( part_owns ) );
    }
    else {
      STKUNIT_ASSERT( ! bulk.bucket(node).member( part_owns ) );
    }

#if 0
    //// DEBUG

    std::cout << "p_rank = " << p_rank << "; i = " << i << std::endl;
    std::cout << " node of interest " << node << std::endl;
#ifdef USE_STK_MESH_IMPL_PARTITION
    std::cout << " its partition " << *node.bucket().getPartition() << std::endl;
#else
    std::cout << " its bucket " << node.bucket() << std::endl;
#endif
    std::cout << " epart_0 " << *epart_0 << " " << epart_0->mesh_meta_data_ordinal() << std::endl;
    std::cout << " epart_1 " << *epart_1 << " " << epart_1->mesh_meta_data_ordinal() << std::endl;

    //// GUBED
#endif

    STKUNIT_ASSERT( bulk.bucket(node).member( part_univ ) );
    STKUNIT_ASSERT( bulk.bucket(node).member( *epart_0 ) );
    STKUNIT_ASSERT( bulk.bucket(node).member( *epart_1 ) );
  }
}
