// Copyright (c) 2002-present, The OpenMS Team -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg $
// $Authors: David Wojnar $
// --------------------------------------------------------------------------

#pragma once

#include <OpenMS/DATASTRUCTURES/DefaultParamHandler.h>
#include <OpenMS/KERNEL/ConsensusMap.h>
#include <OpenMS/KERNEL/FeatureMap.h>
#include <OpenMS/FORMAT/FASTAFile.h>
#include <OpenMS/METADATA/PeptideIdentification.h>
#include <OpenMS/METADATA/ProteinIdentification.h>

namespace OpenMS
{
  /**
    @brief Helper class for peptide and protein quantification based on feature data annotated with IDs

    This class is used by @ref TOPP_ProteinResolver . See there for further documentation.

    @ingroup Analysis_ID

  */
  class OPENMS_DLLAPI ProteinResolver :
    public DefaultParamHandler
  {

public:

    //default constructor
    ProteinResolver();

    //copy constructor
    ProteinResolver(const ProteinResolver & rhs);

    //assignment operator
    ProteinResolver & operator=(const ProteinResolver & rhs);

    //destructor
    ~ProteinResolver() override;


    struct ProteinEntry;
    struct PeptideEntry;
    struct ISDGroup;
    struct MSDGroup;
    struct ResolverResult;

    /// represents a protein from FASTA file
    struct ProteinEntry
    {
      std::list<PeptideEntry *> peptides;
      bool traversed;
      FASTAFile::FASTAEntry * fasta_entry;
      enum type  {primary, secondary, primary_indistinguishable, secondary_indistinguishable} protein_type;
      double weight;    //monoisotopic
      float coverage;    //in percent
      //if Protein is indistinguishable all his fellows are in the list indis
      std::list<ProteinEntry *> indis;
      Size index;
      Size  msd_group;     //index
      Size  isd_group;     //index
      Size number_of_experimental_peptides;
    };

    /// represents a peptide. First in silico. If experimental is set to true it is MS/MS derived.
    struct PeptideEntry
    {
      std::list<ProteinEntry *> proteins;
      bool traversed;
      String sequence;
      Size peptide_identification;
      Size peptide_hit;
      Size index;
      Size  msd_group;     //index
      Size isd_group;     //index
      bool experimental;
      float intensity;
      String origin;
    };

    /// representation of an msd group. Contains peptides, proteins and a pointer to its ISD group
    struct MSDGroup
    {
      std::list<ProteinEntry *> proteins;
      std::list<PeptideEntry *> peptides;
      Size index;
      ISDGroup * isd_group;
      Size number_of_decoy;
      Size number_of_target;
      Size number_of_target_plus_decoy;
      float intensity;     ///< intensity of the MSD Group. Defined as the median of the peptide intensities.
    };

    struct ISDGroup
    {
      std::list<ProteinEntry *> proteins;
      std::list<PeptideEntry *> peptides;
      Size index;
      std::list<Size> msd_groups;
    };

    struct ResolverResult
    {
      String identifier;
      std::vector<ISDGroup> * isds;
      std::vector<MSDGroup> * msds;
      std::vector<ProteinEntry> * protein_entries;
      std::vector<PeptideEntry> * peptide_entries;
      std::vector<Size> * reindexed_peptides;
      std::vector<Size> * reindexed_proteins;
      enum type  {PeptideIdent, Consensus} input_type;
      std::vector<PeptideIdentification> * peptide_identification;
      ConsensusMap * consensus_map;
    };

    /**
      @brief Computing protein groups from peptide identifications OR consensus map.

      Computes ISD and MSD groups.

      @param consensus ConsensusMap in case consensusXML file is given as input
    */
    void resolveConsensus(ConsensusMap & consensus);

    /**
      @brief Computing protein groups from peptide identifications OR consensus map.

      Computes ISD and MSD groups.

      @param peptide_identifications Vector of PeptideIdentification in case idXML is given as input
    */
    void resolveID(std::vector<PeptideIdentification> & peptide_identifications);

    /**
      @brief brief

      @param msd_groups
      @param consensus
    */
    void countTargetDecoy(std::vector<MSDGroup> & msd_groups, ConsensusMap & consensus);

    /**
      @brief brief

      @param msd_groups
      @param peptide_nodes
    */
    void countTargetDecoy(std::vector<MSDGroup> & msd_groups, std::vector<PeptideIdentification> & peptide_nodes);

    void clearResult();

    void setProteinData(std::vector<FASTAFile::FASTAEntry> & protein_data);

    const  std::vector<ResolverResult> & getResults();

    /// overloaded functions -- return a const reference to a PeptideIdentification object or a peptideHit either from a consensusMap or a vector<PeptideIdentification>
    static const PeptideIdentification & getPeptideIdentification(const ConsensusMap & consensus, const PeptideEntry * peptide);
    static const PeptideHit & getPeptideHit(const ConsensusMap & consensus, const PeptideEntry * peptide);
    static const PeptideIdentification & getPeptideIdentification(const std::vector<PeptideIdentification> & peptide_nodes, const PeptideEntry * peptide);
    static const PeptideHit & getPeptideHit(const std::vector<PeptideIdentification> & peptide_nodes, const PeptideEntry * peptide);

private:

    std::vector<ResolverResult> resolver_result_;
    std::vector<FASTAFile::FASTAEntry> protein_data_;

    void computeIntensityOfMSD_(std::vector<MSDGroup> & msd_groups);

    /// traverse protein and peptide nodes for building MSD groups
    void traverseProtein_(ProteinEntry * prot_node, MSDGroup & group);
    void traversePeptide_(PeptideEntry * pep_node, MSDGroup & group);
    /// searches given sequence in all nodes and returns its index or nodes.size() if not found.
    Size findPeptideEntry_(String seq, std::vector<PeptideEntry> & nodes);
    /// helper function for findPeptideEntry.
    Size binarySearchNodes_(String & seq, std::vector<PeptideEntry> & nodes, Size start, Size end);
    /// includes all MS/MS derived peptides into the graph --idXML
    Size includeMSMSPeptides_(std::vector<PeptideIdentification> & peptide_identifications, std::vector<PeptideEntry> & peptide_nodes);
    /// TODO include run information for each peptide
    /// includes all MS/MS derived peptides into the graph --consensusXML
    Size includeMSMSPeptides_(ConsensusMap & consensus, std::vector<PeptideEntry> & peptide_nodes);
    /// Proteins and Peptides get reindexed, based on whether they belong to msd groups or not. Indexes of Proteins which are in an ISD group but in none of the MSD groups will not be used anymore.
    void reindexingNodes_(std::vector<MSDGroup> & msd_groups, std::vector<Size> & reindexed_proteins, std::vector<Size> & reindexed_peptides);
    /// marks Proteins which have a unique peptide as primary. Uses reindexed vector, thus reindexingNodes has to be called before.
    void primaryProteins_(std::vector<PeptideEntry> & peptide_nodes, std::vector<Size> & reindexed_peptides);
    void buildingMSDGroups_(std::vector<MSDGroup> & msd_groups, std::vector<ISDGroup> & isd_groups);
    void buildingISDGroups_(std::vector<ProteinEntry> & protein_nodes, std::vector<PeptideEntry> & peptide_nodes,
                            std::vector<ISDGroup> & isd_groups);
    // disabled/buggy
    //ProteinResolver::indistinguishableProteins(vector<MSDGroup>& msd_groups);

  }; // class

} // namespace

