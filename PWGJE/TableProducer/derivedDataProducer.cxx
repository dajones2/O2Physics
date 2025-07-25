// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

// task to produce a self contained data format for jet analyses from the full AO2D
//
/// \author Nima Zardoshti <nima.zardoshti@cern.ch>

#include "PWGDQ/DataModel/ReducedInfoTables.h"
#include "PWGHF/DataModel/DerivedTables.h"
#include "PWGHF/Utils/utilsBfieldCCDB.h"
#include "PWGJE/Core/JetDQUtilities.h"
#include "PWGJE/Core/JetDerivedDataUtilities.h"
#include "PWGJE/Core/JetV0Utilities.h"
#include "PWGJE/DataModel/EMCALClusters.h"
#include "PWGJE/DataModel/EMCALMatchedCollisions.h"
#include "PWGJE/DataModel/JetReducedData.h"
#include "PWGJE/DataModel/JetReducedDataDQ.h"
#include "PWGJE/DataModel/JetReducedDataHF.h"
#include "PWGJE/DataModel/JetReducedDataV0.h"
#include "PWGLF/DataModel/LFStrangenessTables.h"
#include "PWGLF/DataModel/mcCentrality.h"

#include "Common/CCDB/ctpRateFetcher.h"
#include "Common/Core/RecoDecay.h"
#include "Common/Core/trackUtilities.h"
#include "Common/DataModel/Centrality.h"
#include "Common/DataModel/CollisionAssociationTables.h"
#include "Common/DataModel/EventSelection.h"
#include "Common/DataModel/Multiplicity.h"
#include "Common/DataModel/TrackSelectionTables.h"
#include "EventFiltering/Zorro.h"

#include "CCDB/BasicCCDBManager.h"
#include "DetectorsBase/Propagator.h"
#include "Framework/ASoA.h"
#include "Framework/AnalysisDataModel.h"
#include "Framework/AnalysisTask.h"
#include "Framework/O2DatabasePDGPlugin.h"
#include "ReconstructionDataFormats/Vertex.h"
#include <CommonConstants/MathConstants.h>
#include <DetectorsBase/MatLayerCylSet.h>
#include <Framework/AnalysisHelpers.h>
#include <Framework/Configurable.h>
#include <Framework/InitContext.h>
#include <Framework/runDataProcessing.h>
#include <ReconstructionDataFormats/DCA.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <map>
#include <string>
#include <utility>
#include <vector>

using namespace o2;
using namespace o2::framework;
using namespace o2::framework::expressions;

struct JetDerivedDataProducerTask {
  struct : ProducesGroup {
    Produces<aod::BCCounts> bcCountsTable;
    Produces<aod::CollisionCounts> collisionCountsTable;
    Produces<aod::JDummys> jDummysTable;
    Produces<aod::JBCs> jBCsTable;
    Produces<aod::JBCPIs> jBCParentIndexTable;
    Produces<aod::JCollisions> jCollisionsTable;
    Produces<aod::JCollisionMcInfos> jCollisionMcInfosTable;
    Produces<aod::JCollisionPIs> jCollisionsParentIndexTable;
    Produces<aod::JCollisionBCs> jCollisionsBunchCrossingIndexTable;
    Produces<aod::JEMCCollisionLbs> jCollisionsEMCalLabelTable;
    Produces<aod::JMcCollisionLbs> jMcCollisionsLabelTable;
    Produces<aod::JMcCollisions> jMcCollisionsTable;
    Produces<aod::JMcCollisionPIs> jMcCollisionsParentIndexTable;
    Produces<aod::JTracks> jTracksTable;
    Produces<aod::JTrackExtras> jTracksExtraTable;
    Produces<aod::JEMCTracks> jTracksEMCalTable;
    Produces<aod::JTrackPIs> jTracksParentIndexTable;
    Produces<aod::JMcTrackLbs> jMcTracksLabelTable;
    Produces<aod::JMcParticles> jMcParticlesTable;
    Produces<aod::JMcParticlePIs> jParticlesParentIndexTable;
    Produces<aod::JClusters> jClustersTable;
    Produces<aod::JClusterPIs> jClustersParentIndexTable;
    Produces<aod::JClusterTracks> jClustersMatchedTracksTable;
    Produces<aod::JMcClusterLbs> jMcClustersLabelTable;
    Produces<aod::JD0CollisionIds> jD0CollisionIdsTable;
    Produces<aod::JD0McCollisionIds> jD0McCollisionIdsTable;
    Produces<aod::JD0Ids> jD0IdsTable;
    Produces<aod::JD0PIds> jD0ParticleIdsTable;
    Produces<aod::JDplusCollisionIds> jDplusCollisionIdsTable;
    Produces<aod::JDplusMcCollisionIds> jDplusMcCollisionIdsTable;
    Produces<aod::JDplusIds> jDplusIdsTable;
    Produces<aod::JDplusPIds> jDplusParticleIdsTable;
    Produces<aod::JDstarCollisionIds> jDstarCollisionIdsTable;
    Produces<aod::JDstarMcCollisionIds> jDstarMcCollisionIdsTable;
    Produces<aod::JDstarIds> jDstarIdsTable;
    Produces<aod::JDstarPIds> jDstarParticleIdsTable;
    Produces<aod::JLcCollisionIds> jLcCollisionIdsTable;
    Produces<aod::JLcMcCollisionIds> jLcMcCollisionIdsTable;
    Produces<aod::JLcIds> jLcIdsTable;
    Produces<aod::JLcPIds> jLcParticleIdsTable;
    Produces<aod::JB0CollisionIds> jB0CollisionIdsTable;
    Produces<aod::JB0McCollisionIds> jB0McCollisionIdsTable;
    Produces<aod::JB0Ids> jB0IdsTable;
    Produces<aod::JB0PIds> jB0ParticleIdsTable;
    Produces<aod::JBplusCollisionIds> jBplusCollisionIdsTable;
    Produces<aod::JBplusMcCollisionIds> jBplusMcCollisionIdsTable;
    Produces<aod::JBplusIds> jBplusIdsTable;
    Produces<aod::JBplusPIds> jBplusParticleIdsTable;
    Produces<aod::JV0Ids> jV0IdsTable;
    Produces<aod::JV0McCollisions> jV0McCollisionsTable;
    Produces<aod::JV0McCollisionIds> jV0McCollisionIdsTable;
    Produces<aod::JV0Mcs> jV0McsTable;
    Produces<aod::JV0McIds> jV0McIdsTable;
    Produces<aod::JDielectronCollisionIds> jDielectronCollisionIdsTable;
    Produces<aod::JDielectronIds> jDielectronIdsTable;
    Produces<aod::JDielectronMcCollisions> jDielectronMcCollisionsTable;
    Produces<aod::JDielectronMcCollisionIds> jDielectronMcCollisionIdsTable;
    Produces<aod::JDielectronMcRCollDummys> JDielectronMcRCollDummysTable;
    Produces<aod::JDielectronMcs> jDielectronMcsTable;
    Produces<aod::JDielectronMcIds> jDielectronMcIdsTable;
  } products;

  Configurable<std::string> ccdbUrl{"ccdbUrl", "http://alice-ccdb.cern.ch", "url of the ccdb repository"};
  Configurable<std::string> ccdbPathLut{"ccdbPathLut", "GLO/Param/MatLUT", "Path for LUT parametrization"};
  Configurable<std::string> ccdbPathGrp{"ccdbPathGrp", "GLO/GRP/GRP", "Path of the grp file (Run 2)"};
  Configurable<std::string> ccdbPathGrpMag{"ccdbPathGrpMag", "GLO/Config/GRPMagField", "CCDB path of the GRPMagField object (Run 3)"};
  Configurable<float> dcaZMax{"dcaZMax", 0.2, "maximum DCAZ selection for tracks - only applied for reassociation"};

  Configurable<std::string> ccdbURL{"ccdb-url", "http://alice-ccdb.cern.ch", "url of the ccdb repository"};
  Configurable<bool> includeTriggers{"includeTriggers", false, "fill the collision information with software trigger decisions"};
  Configurable<bool> includeHadronicRate{"includeHadronicRate", true, "fill the collision information with the hadronic rate"};

  Preslice<aod::EMCALClusterCells> perClusterCells = aod::emcalclustercell::emcalclusterId;
  Preslice<aod::EMCALMatchedTracks> perClusterTracks = aod::emcalclustercell::emcalclusterId;
  Preslice<aod::TrackAssoc> perCollisionTrackIndices = aod::track_association::collisionId;

  std::map<std::pair<int32_t, int32_t>, int32_t> trackCollisionMapping;
  Service<o2::ccdb::BasicCCDBManager> ccdb;
  o2::base::MatLayerCylSet* lut;
  o2::base::Propagator::MatCorrType noMatCorr = o2::base::Propagator::MatCorrType::USEMatCorrNONE;
  Service<o2::framework::O2DatabasePDG> pdgDatabase;
  Zorro triggerDecider;

  ctpRateFetcher rateFetcher;
  int runNumber;
  float hadronicRate;
  bool withCollisionAssociator;
  void init(InitContext const&)
  {
    hadronicRate = -1.0;
    if (doprocessTracksWithCollisionAssociator || includeHadronicRate || includeTriggers) {
      ccdb->setURL(ccdbUrl);
      ccdb->setCaching(true);
      ccdb->setLocalObjectValidityChecking();
      runNumber = 0;
      if (doprocessTracksWithCollisionAssociator) {
        withCollisionAssociator = true;
        lut = o2::base::MatLayerCylSet::rectifyPtrFromFile(ccdb->get<o2::base::MatLayerCylSet>(ccdbPathLut));
      } else {
        withCollisionAssociator = false;
      }
    }
  }

  void processClearMaps(aod::Collisions const& collisions)
  {
    trackCollisionMapping.clear();
    if (!doprocessMcCollisionLabels) {
      for (int i = 0; i < collisions.size(); i++) {
        products.jCollisionMcInfosTable(-1.0, jetderiveddatautilities::JCollisionSubGeneratorId::none); // fill a dummy weights table if not MC
      }
    }
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processClearMaps, "clears all maps", true);

  void processBunchCrossings(soa::Join<aod::BCs, aod::Timestamps, aod::BcSels>::iterator const& bc)
  {
    products.jBCsTable(bc.runNumber(), bc.globalBC(), bc.timestamp(), bc.alias_raw(), bc.selection_raw());
    products.jBCParentIndexTable(bc.globalIndex());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processBunchCrossings, "produces derived bunch crossing table", false);

  void processCollisions(soa::Join<aod::Collisions, aod::EvSels, aod::FV0Mults, aod::FT0Mults, aod::CentFV0As, aod::CentFT0As, aod::CentFT0Cs, aod::CentFT0Ms, aod::CentFT0CVariant1s>::iterator const& collision, soa::Join<aod::BCs, aod::Timestamps> const&)
  {
    auto bc = collision.bc_as<soa::Join<aod::BCs, aod::Timestamps>>();
    if (includeHadronicRate) {
      if (runNumber != bc.runNumber()) {
        runNumber = bc.runNumber();
        hadronicRate = rateFetcher.fetch(ccdb.service, bc.timestamp(), runNumber, "ZNC hadronic") * 0.001;
      }
    }
    uint64_t triggerBit = 0;
    if (includeTriggers) {
      triggerDecider.initCCDB(ccdb.service, bc.runNumber(), bc.timestamp(), jetderiveddatautilities::JTriggerMasks);
      triggerBit = jetderiveddatautilities::setTriggerSelectionBit(triggerDecider.getTriggerOfInterestResults(bc.globalBC()));
    }
    products.jCollisionsTable(collision.posX(), collision.posY(), collision.posZ(), collision.multFV0A(), collision.multFV0C(), collision.multFT0A(), collision.multFT0C(), collision.centFV0A(), -1.0, collision.centFT0A(), collision.centFT0C(), collision.centFT0M(), collision.centFT0CVariant1(), hadronicRate, collision.trackOccupancyInTimeRange(), jetderiveddatautilities::setEventSelectionBit(collision), collision.alias_raw(), triggerBit); // note change multFT0C to multFT0M when problems with multFT0A are fixed
    products.jCollisionsParentIndexTable(collision.globalIndex());
    products.jCollisionsBunchCrossingIndexTable(collision.bcId());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processCollisions, "produces derived collision tables", true);

  void processCollisionsWithoutCentralityAndMultiplicity(soa::Join<aod::Collisions, aod::EvSels>::iterator const& collision, soa::Join<aod::BCs, aod::Timestamps> const&)
  {
    uint64_t triggerBit = 0;
    if (includeTriggers) {
      auto bc = collision.bc_as<soa::Join<aod::BCs, aod::Timestamps>>();
      triggerDecider.initCCDB(ccdb.service, bc.runNumber(), bc.timestamp(), jetderiveddatautilities::JTriggerMasks);
      triggerBit = jetderiveddatautilities::setTriggerSelectionBit(triggerDecider.getTriggerOfInterestResults(bc.globalBC()));
    }
    products.jCollisionsTable(collision.posX(), collision.posY(), collision.posZ(), -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1, jetderiveddatautilities::setEventSelectionBit(collision), collision.alias_raw(), triggerBit);
    products.jCollisionsParentIndexTable(collision.globalIndex());
    products.jCollisionsBunchCrossingIndexTable(collision.bcId());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processCollisionsWithoutCentralityAndMultiplicity, "produces derived collision tables without centrality or multiplicity", false);

  void processCollisionsRun2(soa::Join<aod::Collisions, aod::EvSels, aod::FT0Mults, aod::CentRun2V0As, aod::CentRun2V0Ms>::iterator const& collision)
  {
    products.jCollisionsTable(collision.posX(), collision.posY(), collision.posZ(), -1.0, -1.0, -1.0, -1.0, collision.centRun2V0A(), collision.centRun2V0M(), -1.0, -1.0, -1.0, -1.0, 1.0, -1, jetderiveddatautilities::setEventSelectionBit(collision), collision.alias_raw(), 0); // note change multFT0C to multFT0M when problems with multFT0A are fixed
    products.jCollisionsParentIndexTable(collision.globalIndex());
    products.jCollisionsBunchCrossingIndexTable(collision.bcId());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processCollisionsRun2, "produces derived collision tables for Run 2 data", false);

  void processCollisionsALICE3(aod::Collision const& collision)
  {
    products.jCollisionsTable(collision.posX(), collision.posY(), collision.posZ(), -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1, -1.0, 0, 0);
    products.jCollisionsParentIndexTable(collision.globalIndex());
    products.jCollisionsBunchCrossingIndexTable(-1);
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processCollisionsALICE3, "produces derived collision tables for ALICE 3 simulations", false);

  void processWithoutEMCalCollisionLabels(aod::Collision const&)
  {
    products.jCollisionsEMCalLabelTable(false, false);
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processWithoutEMCalCollisionLabels, "produces dummy derived collision labels for EMCal", true);

  void processEMCalCollisionLabels(aod::EMCALMatchedCollision const& collision)
  {
    products.jCollisionsEMCalLabelTable(collision.ambiguous(), collision.isemcreadout());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processEMCalCollisionLabels, "produces derived collision labels for EMCal", false);

  void processMcCollisionLabels(soa::Join<aod::Collisions, aod::McCollisionLabels>::iterator const& collision, aod::McCollisions const&)
  {
    products.jMcCollisionsLabelTable(collision.mcCollisionId()); // collision.mcCollisionId() returns -1 if collision has no associated mcCollision
    if (collision.has_mcCollision()) {
      products.jCollisionMcInfosTable(collision.mcCollision().weight(), collision.mcCollision().getSubGeneratorId());
    } else {
      products.jCollisionMcInfosTable(0.0, jetderiveddatautilities::JCollisionSubGeneratorId::none);
    }
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processMcCollisionLabels, "produces derived MC collision labels table", false);

  void processMcCollisions(soa::Join<aod::McCollisions, aod::HepMCXSections, aod::MultsExtraMC, aod::McCentFV0As, aod::McCentFT0As, aod::McCentFT0Cs, aod::McCentFT0Ms>::iterator const& mcCollision)
  {
    products.jMcCollisionsTable(mcCollision.posX(), mcCollision.posY(), mcCollision.posZ(), mcCollision.multMCFV0A(), mcCollision.multMCFT0A(), mcCollision.multMCFT0C(), mcCollision.centFV0A(), mcCollision.centFT0A(), mcCollision.centFT0C(), mcCollision.centFT0M(), mcCollision.weight(), mcCollision.getSubGeneratorId(), mcCollision.accepted(), mcCollision.attempted(), mcCollision.xsectGen(), mcCollision.xsectErr(), mcCollision.ptHard());
    products.jMcCollisionsParentIndexTable(mcCollision.globalIndex());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processMcCollisions, "produces derived MC collision table", false);

  void processMcCollisionsWithoutCentralityAndMultiplicity(soa::Join<aod::McCollisions, aod::HepMCXSections>::iterator const& mcCollision)
  {
    products.jMcCollisionsTable(mcCollision.posX(), mcCollision.posY(), mcCollision.posZ(), -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, mcCollision.weight(), mcCollision.getSubGeneratorId(), mcCollision.accepted(), mcCollision.attempted(), mcCollision.xsectGen(), mcCollision.xsectErr(), mcCollision.ptHard());
    products.jMcCollisionsParentIndexTable(mcCollision.globalIndex());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processMcCollisionsWithoutCentralityAndMultiplicity, "produces derived MC collision table without centraility and multiplicity", false);

  void processMcCollisionsWithoutXsection(soa::Join<aod::McCollisions, aod::MultsExtraMC, aod::McCentFV0As, aod::McCentFT0As, aod::McCentFT0Cs, aod::McCentFT0Ms>::iterator const& mcCollision)
  {
    products.jMcCollisionsTable(mcCollision.posX(), mcCollision.posY(), mcCollision.posZ(), mcCollision.multMCFV0A(), mcCollision.multMCFT0A(), mcCollision.multMCFT0C(), mcCollision.centFV0A(), mcCollision.centFT0A(), mcCollision.centFT0C(), mcCollision.centFT0M(), mcCollision.weight(), mcCollision.getSubGeneratorId(), 1, 1, 1.0, 1.0, 999.0);
    products.jMcCollisionsParentIndexTable(mcCollision.globalIndex());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processMcCollisionsWithoutXsection, "produces derived MC collision table without cross section information", false);

  void processMcCollisionsWithoutCentralityAndMultiplicityAndXsection(aod::McCollision const& mcCollision)
  {
    products.jMcCollisionsTable(mcCollision.posX(), mcCollision.posY(), mcCollision.posZ(), -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, mcCollision.weight(), mcCollision.getSubGeneratorId(), 1, 1, 1.0, 1.0, 999.0);
    products.jMcCollisionsParentIndexTable(mcCollision.globalIndex());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processMcCollisionsWithoutCentralityAndMultiplicityAndXsection, "produces derived MC collision table without centrality, multiplicity and cross section information", false);

  void processTracks(soa::Join<aod::Tracks, aod::TracksExtra, aod::TracksCov, aod::TracksDCA, aod::TracksDCACov, aod::TrackSelection, aod::TrackSelectionExtension>::iterator const& track, aod::Collisions const&)
  {
    products.jTracksTable(track.collisionId(), track.pt(), track.eta(), track.phi(), jetderiveddatautilities::setTrackSelectionBit(track, track.dcaZ(), dcaZMax));
    auto trackParCov = getTrackParCov(track);
    auto xyzTrack = trackParCov.getXYZGlo();
    float sigmaDCAXYZ2;
    float dcaXYZ = getDcaXYZ(track, &sigmaDCAXYZ2);
    float dcaX = -99.0;
    float dcaY = -99.0;
    if (track.collisionId() >= 0) {
      auto const& collision = track.collision_as<aod::Collisions>();
      dcaX = xyzTrack.X() - collision.posX();
      dcaY = xyzTrack.Y() - collision.posY();
    }

    products.jTracksExtraTable(dcaX, dcaY, track.dcaZ(), track.dcaXY(), dcaXYZ, std::sqrt(track.sigmaDcaZ2()), std::sqrt(track.sigmaDcaXY2()), std::sqrt(sigmaDCAXYZ2), track.sigma1Pt()); // why is this getSigmaZY
    products.jTracksParentIndexTable(track.globalIndex());
    trackCollisionMapping[{track.globalIndex(), track.collisionId()}] = products.jTracksTable.lastIndex();
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processTracks, "produces derived track table", true);

  void processTracksWithCollisionAssociator(aod::Collisions const& collisions, soa::Join<aod::BCs, aod::Timestamps> const&, soa::Join<aod::Tracks, aod::TracksExtra, aod::TracksCov, aod::TracksDCA, aod::TracksDCACov, aod::TrackSelection, aod::TrackSelectionExtension> const&, aod::TrackAssoc const& assocCollisions)
  {
    runNumber = 0;
    for (auto const& collision : collisions) {
      auto collisionTrackIndices = assocCollisions.sliceBy(perCollisionTrackIndices, collision.globalIndex());
      for (auto const& collisionTrackIndex : collisionTrackIndices) {
        auto track = collisionTrackIndex.track_as<soa::Join<aod::Tracks, aod::TracksExtra, aod::TracksCov, aod::TracksDCA, aod::TracksDCACov, aod::TrackSelection, aod::TrackSelectionExtension>>();
        auto trackParCov = getTrackParCov(track);
        if (track.collisionId() == collision.globalIndex()) {
          products.jTracksTable(collision.globalIndex(), track.pt(), track.eta(), track.phi(), jetderiveddatautilities::setTrackSelectionBit(track, track.dcaZ(), dcaZMax));
          products.jTracksParentIndexTable(track.globalIndex());
          auto xyzTrack = trackParCov.getXYZGlo();
          float sigmaDCAXYZ2;
          float dcaXYZ = getDcaXYZ(track, &sigmaDCAXYZ2);
          products.jTracksExtraTable(xyzTrack.X() - collision.posX(), xyzTrack.Y() - collision.posY(), track.dcaZ(), track.dcaXY(), dcaXYZ, std::sqrt(track.sigmaDcaZ2()), std::sqrt(track.sigmaDcaXY2()), std::sqrt(sigmaDCAXYZ2), track.sigma1Pt()); // why is this getSigmaZY
        } else {
          auto bc = collision.bc_as<soa::Join<aod::BCs, aod::Timestamps>>();
          initCCDB(bc, runNumber, ccdb, doprocessCollisionsRun2 ? ccdbPathGrp : ccdbPathGrpMag, lut, doprocessCollisionsRun2);
          o2::dataformats::DCA dcaCovInfo;
          dcaCovInfo.set(-999., -999., -999., -999., -999.);
          o2::dataformats::VertexBase collisionInfo;
          collisionInfo.setPos({collision.posX(), collision.posY(), collision.posZ()});
          collisionInfo.setCov(collision.covXX(), collision.covXY(), collision.covYY(), collision.covXZ(), collision.covYZ(), collision.covZZ());
          o2::base::Propagator::Instance()->propagateToDCABxByBz(collisionInfo, trackParCov, 2.f, noMatCorr, &dcaCovInfo);
          products.jTracksTable(collision.globalIndex(), trackParCov.getPt(), trackParCov.getEta(), trackParCov.getPhi(), jetderiveddatautilities::setTrackSelectionBit(track, dcaCovInfo.getZ(), dcaZMax)); // only qualitytracksWDCA are a reliable selection
          products.jTracksParentIndexTable(track.globalIndex());
          auto xyzTrack = trackParCov.getXYZGlo();
          float dcaXY = dcaCovInfo.getY();
          float dcaZ = dcaCovInfo.getZ();
          float dcaXYZ = std::sqrt(dcaXY * dcaXY + dcaZ * dcaZ);
          float covYY = dcaCovInfo.getSigmaY2();
          float covZZ = dcaCovInfo.getSigmaZ2();
          float covYZ = dcaCovInfo.getSigmaYZ();
          float sigmaDCAXYZ;
          if (dcaXYZ < o2::constants::math::Almost0) {
            sigmaDCAXYZ = o2::constants::math::VeryBig; // Protection against division by zero
          } else {
            sigmaDCAXYZ = covYY * (2.f * dcaXY / dcaXYZ) * (2.f * dcaXY / dcaXYZ) + covZZ * (2.f * dcaZ / dcaXYZ) * (2.f * dcaZ / dcaXYZ) + 2.f * covYZ * (2.f * dcaXY / dcaXYZ) * (2.f * dcaZ / dcaXYZ);
          }
          products.jTracksExtraTable(xyzTrack.X() - collision.posX(), xyzTrack.Y() - collision.posY(), dcaZ, dcaXY, dcaXYZ, std::sqrt(covZZ), std::sqrt(covYY), std::sqrt(sigmaDCAXYZ), std::sqrt(trackParCov.getSigma1Pt2()));
        }
        trackCollisionMapping[{track.globalIndex(), collision.globalIndex()}] = products.jTracksTable.lastIndex();
      }
    }
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processTracksWithCollisionAssociator, "produces derived track table taking into account track-to-collision associations", false);

  void processTracksRun2(soa::Join<aod::Tracks, aod::TracksExtra, aod::TracksCov, aod::TracksDCA, aod::TrackSelection, aod::TrackSelectionExtension>::iterator const& track)
  {
    // TracksDCACov table is not yet available for Run 2 converted data. Remove this process function and use only processTracks when that becomes available.
    products.jTracksTable(track.collisionId(), track.pt(), track.eta(), track.phi(), jetderiveddatautilities::setTrackSelectionBit(track, track.dcaZ(), dcaZMax));
    float sigmaDCAXYZ2 = 0.0;
    float dcaXYZ = getDcaXYZ(track, &sigmaDCAXYZ2);
    float dcaX = -99.0;
    float dcaY = -99.0;

    products.jTracksExtraTable(dcaX, dcaY, track.dcaZ(), track.dcaXY(), dcaXYZ, std::sqrt(1.), std::sqrt(1.), std::sqrt(sigmaDCAXYZ2), track.sigma1Pt()); // dummy values - will be fixed when TracksDCACov table is available for Run 2
    products.jTracksParentIndexTable(track.globalIndex());
    trackCollisionMapping[{track.globalIndex(), track.collisionId()}] = products.jTracksTable.lastIndex();
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processTracksRun2, "produces derived track table for Run2 AO2Ds", false);

  void processMcTrackLabels(soa::Join<aod::Tracks, aod::McTrackLabels>::iterator const& track)
  {
    if (track.has_mcParticle()) {
      products.jMcTracksLabelTable(track.mcParticleId());
    } else {
      products.jMcTracksLabelTable(-1);
    }
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processMcTrackLabels, "produces derived track labels table", false);

  void processMcTrackLabelsWithCollisionAssociator(aod::Collisions const& collisions, soa::Join<aod::Tracks, aod::McTrackLabels> const&, aod::TrackAssoc const& assocCollisions)
  {
    for (auto const& collision : collisions) {
      auto collisionTrackIndices = assocCollisions.sliceBy(perCollisionTrackIndices, collision.globalIndex());
      for (auto const& collisionTrackIndex : collisionTrackIndices) {
        auto track = collisionTrackIndex.track_as<soa::Join<aod::Tracks, aod::McTrackLabels>>();
        if (track.collisionId() == collision.globalIndex() && track.has_mcParticle()) {
          products.jMcTracksLabelTable(track.mcParticleId());
        } else {
          products.jMcTracksLabelTable(-1);
        }
      }
    }
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processMcTrackLabelsWithCollisionAssociator, "produces derived track labels table taking into account track-to-collision associations", false);

  void processParticles(aod::McParticle const& particle)
  {
    std::vector<int32_t> mothersId;
    if (particle.has_mothers()) {
      auto mothersIdTemps = particle.mothersIds();
      for (auto mothersIdTemp : mothersIdTemps) {
        mothersId.push_back(mothersIdTemp);
      }
    }
    int daughtersId[2] = {-1, -1};
    auto i = 0;
    if (particle.has_daughters()) {
      for (auto daughterId : particle.daughtersIds()) {
        if (i > 1) {
          break;
        }
        daughtersId[i] = daughterId;
        i++;
      }
    }
    products.jMcParticlesTable(particle.mcCollisionId(), particle.pt(), particle.eta(), particle.phi(), particle.y(), particle.e(), particle.pdgCode(), particle.getGenStatusCode(), particle.getHepMCStatusCode(), particle.isPhysicalPrimary(), mothersId, daughtersId);
    products.jParticlesParentIndexTable(particle.globalIndex());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processParticles, "produces derived parrticle table", false);

  void processClusters(aod::Collision const&, aod::EMCALClusters const& clusters, aod::EMCALClusterCells const& cells, aod::Calos const&, aod::EMCALMatchedTracks const& matchedTracks, soa::Join<aod::Tracks, aod::TracksExtra> const&)
  {

    for (auto cluster : clusters) {

      auto const clusterCells = cells.sliceBy(perClusterCells, cluster.globalIndex());

      float leadingCellEnergy = -1.0;
      float subleadingCellEnergy = -1.0;
      float cellAmplitude = -1.0;
      int leadingCellNumber = -1;
      int subleadingCellNumber = -1;
      int cellNumber = -1;
      for (auto const& clutserCell : clusterCells) {
        cellAmplitude = clutserCell.calo().amplitude();
        cellNumber = clutserCell.calo().cellNumber();
        if (cellAmplitude > subleadingCellEnergy) {
          subleadingCellEnergy = cellAmplitude;
          subleadingCellNumber = cellNumber;
        }
        if (subleadingCellEnergy > leadingCellEnergy) {
          std::swap(leadingCellEnergy, subleadingCellEnergy);
          std::swap(leadingCellNumber, subleadingCellNumber);
        }
      }

      products.jClustersTable(cluster.collisionId(), cluster.id(), cluster.energy(), cluster.coreEnergy(), cluster.rawEnergy(), cluster.eta(), cluster.phi(), cluster.m02(), cluster.m20(), cluster.nCells(), cluster.time(), cluster.isExotic(), cluster.distanceToBadChannel(), cluster.nlm(), cluster.definition(), leadingCellEnergy, subleadingCellEnergy, leadingCellNumber, subleadingCellNumber);
      products.jClustersParentIndexTable(cluster.globalIndex());

      auto const clusterTracks = matchedTracks.sliceBy(perClusterTracks, cluster.globalIndex());
      std::vector<int32_t> clusterTrackIDs;
      for (const auto& clusterTrack : clusterTracks) {
        auto JClusterID = trackCollisionMapping.find({clusterTrack.trackId(), cluster.collisionId()}); // does EMCal use its own associator?
        clusterTrackIDs.push_back(JClusterID->second);
        auto emcTrack = clusterTrack.track_as<soa::Join<aod::Tracks, aod::TracksExtra>>();
        products.jTracksEMCalTable(JClusterID->second, emcTrack.trackEtaEmcal(), emcTrack.trackPhiEmcal());
      }
      products.jClustersMatchedTracksTable(clusterTrackIDs);
    }
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processClusters, "produces derived cluster tables", false);

  void processMcClusterLabels(aod::EMCALMCCluster const& cluster)
  {
    std::vector<int32_t> particleIds;
    for (auto particleId : cluster.mcParticleIds()) {
      particleIds.push_back(particleId);
    }
    std::vector<float> amplitudeA;
    auto amplitudeASpan = cluster.amplitudeA();
    std::copy(amplitudeASpan.begin(), amplitudeASpan.end(), std::back_inserter(amplitudeA));
    products.jMcClustersLabelTable(particleIds, amplitudeA);
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processMcClusterLabels, "produces derived cluster particle label table", false);

  void processD0Collisions(aod::HfD0CollIds::iterator const& D0Collision)
  {
    products.jD0CollisionIdsTable(D0Collision.collisionId());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processD0Collisions, "produces derived index for D0 collisions", false);

  void processD0McCollisions(aod::HfD0McCollIds::iterator const& D0McCollision)
  {
    products.jD0McCollisionIdsTable(D0McCollision.mcCollisionId());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processD0McCollisions, "produces derived index for D0 MC collisions", false);

  void processD0(aod::HfD0Ids::iterator const& D0Candidate, aod::Tracks const&)
  {
    auto JProng0ID = trackCollisionMapping.find({D0Candidate.prong0Id(), D0Candidate.prong0_as<aod::Tracks>().collisionId()});
    auto JProng1ID = trackCollisionMapping.find({D0Candidate.prong1Id(), D0Candidate.prong1_as<aod::Tracks>().collisionId()});
    if (withCollisionAssociator) {
      JProng0ID = trackCollisionMapping.find({D0Candidate.prong0Id(), D0Candidate.collisionId()});
      JProng1ID = trackCollisionMapping.find({D0Candidate.prong1Id(), D0Candidate.collisionId()});
    }
    products.jD0IdsTable(D0Candidate.collisionId(), JProng0ID->second, JProng1ID->second);
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processD0, "produces derived index for D0 candidates", false);

  void processD0MC(aod::HfD0PIds::iterator const& D0Particle)
  {
    products.jD0ParticleIdsTable(D0Particle.mcCollisionId(), D0Particle.mcParticleId());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processD0MC, "produces derived index for D0 particles", false);

  void processDplusCollisions(aod::HfDplusCollIds::iterator const& DplusCollision)
  {
    products.jDplusCollisionIdsTable(DplusCollision.collisionId());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processDplusCollisions, "produces derived index for Dplus collisions", false);

  void processDplusMcCollisions(aod::HfDplusMcCollIds::iterator const& DplusMcCollision)
  {
    products.jDplusMcCollisionIdsTable(DplusMcCollision.mcCollisionId());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processDplusMcCollisions, "produces derived index for Dplus MC collisions", false);

  void processDplus(aod::HfDplusIds::iterator const& DplusCandidate, aod::Tracks const&)
  {
    auto JProng0ID = trackCollisionMapping.find({DplusCandidate.prong0Id(), DplusCandidate.prong0_as<aod::Tracks>().collisionId()});
    auto JProng1ID = trackCollisionMapping.find({DplusCandidate.prong1Id(), DplusCandidate.prong1_as<aod::Tracks>().collisionId()});
    auto JProng2ID = trackCollisionMapping.find({DplusCandidate.prong2Id(), DplusCandidate.prong2_as<aod::Tracks>().collisionId()});
    if (withCollisionAssociator) {
      JProng0ID = trackCollisionMapping.find({DplusCandidate.prong0Id(), DplusCandidate.collisionId()});
      JProng1ID = trackCollisionMapping.find({DplusCandidate.prong1Id(), DplusCandidate.collisionId()});
      JProng2ID = trackCollisionMapping.find({DplusCandidate.prong2Id(), DplusCandidate.collisionId()});
    }
    products.jDplusIdsTable(DplusCandidate.collisionId(), JProng0ID->second, JProng1ID->second, JProng2ID->second);
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processDplus, "produces derived index for Dplus candidates", false);

  void processDplusMC(aod::HfDplusPIds::iterator const& DplusParticle)
  {
    products.jDplusParticleIdsTable(DplusParticle.mcCollisionId(), DplusParticle.mcParticleId());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processDplusMC, "produces derived index for Dplus particles", false);

  void processDstarCollisions(aod::HfDstarCollIds::iterator const& DstarCollision)
  {
    products.jDstarCollisionIdsTable(DstarCollision.collisionId());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processDstarCollisions, "produces derived index for Dstar collisions", false);

  void processDstarMcCollisions(aod::HfDstarMcCollIds::iterator const& DstarMcCollision)
  {
    products.jDstarMcCollisionIdsTable(DstarMcCollision.mcCollisionId());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processDstarMcCollisions, "produces derived index for Dstar MC collisions", false);

  void processDstar(aod::HfDstarIds::iterator const& DstarCandidate, aod::Tracks const&)
  {
    auto JProng0ID = trackCollisionMapping.find({DstarCandidate.prong0Id(), DstarCandidate.prong0_as<aod::Tracks>().collisionId()});
    auto JProng1ID = trackCollisionMapping.find({DstarCandidate.prong1Id(), DstarCandidate.prong1_as<aod::Tracks>().collisionId()});
    auto JProng2ID = trackCollisionMapping.find({DstarCandidate.prong2Id(), DstarCandidate.prong2_as<aod::Tracks>().collisionId()});
    if (withCollisionAssociator) {
      JProng0ID = trackCollisionMapping.find({DstarCandidate.prong0Id(), DstarCandidate.collisionId()});
      JProng1ID = trackCollisionMapping.find({DstarCandidate.prong1Id(), DstarCandidate.collisionId()});
      JProng2ID = trackCollisionMapping.find({DstarCandidate.prong2Id(), DstarCandidate.collisionId()});
    }
    products.jDstarIdsTable(DstarCandidate.collisionId(), JProng0ID->second, JProng1ID->second, JProng2ID->second);
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processDstar, "produces derived index for Dstar candidates", false);

  void processDstarMC(aod::HfDstarPIds::iterator const& DstarParticle)
  {
    products.jDstarParticleIdsTable(DstarParticle.mcCollisionId(), DstarParticle.mcParticleId());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processDstarMC, "produces derived index for Dstar particles", false);

  void processLcCollisions(aod::HfLcCollIds::iterator const& LcCollision)
  {
    products.jLcCollisionIdsTable(LcCollision.collisionId());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processLcCollisions, "produces derived index for Lc collisions", false);

  void processLcMcCollisions(aod::HfLcMcCollIds::iterator const& LcMcCollision)
  {
    products.jLcMcCollisionIdsTable(LcMcCollision.mcCollisionId());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processLcMcCollisions, "produces derived index for Lc MC collisions", false);

  void processLc(aod::HfLcIds::iterator const& LcCandidate, aod::Tracks const&)
  {
    auto JProng0ID = trackCollisionMapping.find({LcCandidate.prong0Id(), LcCandidate.prong0_as<aod::Tracks>().collisionId()});
    auto JProng1ID = trackCollisionMapping.find({LcCandidate.prong1Id(), LcCandidate.prong1_as<aod::Tracks>().collisionId()});
    auto JProng2ID = trackCollisionMapping.find({LcCandidate.prong2Id(), LcCandidate.prong2_as<aod::Tracks>().collisionId()});
    if (withCollisionAssociator) {
      JProng0ID = trackCollisionMapping.find({LcCandidate.prong0Id(), LcCandidate.collisionId()});
      JProng1ID = trackCollisionMapping.find({LcCandidate.prong1Id(), LcCandidate.collisionId()});
      JProng2ID = trackCollisionMapping.find({LcCandidate.prong2Id(), LcCandidate.collisionId()});
    }
    products.jLcIdsTable(LcCandidate.collisionId(), JProng0ID->second, JProng1ID->second, JProng2ID->second);
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processLc, "produces derived index for Lc candidates", false);

  void processLcMC(aod::HfLcPIds::iterator const& LcParticle)
  {
    products.jLcParticleIdsTable(LcParticle.mcCollisionId(), LcParticle.mcParticleId());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processLcMC, "produces derived index for Lc particles", false);

  void processB0Collisions(aod::HfB0CollIds::iterator const& B0Collision)
  {
    products.jB0CollisionIdsTable(B0Collision.collisionId());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processB0Collisions, "produces derived index for B0 collisions", false);

  void processB0McCollisions(aod::HfB0McCollIds::iterator const& B0McCollision)
  {
    products.jB0McCollisionIdsTable(B0McCollision.mcCollisionId());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processB0McCollisions, "produces derived index for B0 MC collisions", false);

  void processB0(aod::HfB0Ids::iterator const& B0Candidate, aod::Tracks const&)
  {
    auto JProng0ID = trackCollisionMapping.find({B0Candidate.prong0Id(), B0Candidate.prong0_as<aod::Tracks>().collisionId()});
    auto JProng1ID = trackCollisionMapping.find({B0Candidate.prong1Id(), B0Candidate.prong1_as<aod::Tracks>().collisionId()});
    auto JProng2ID = trackCollisionMapping.find({B0Candidate.prong2Id(), B0Candidate.prong2_as<aod::Tracks>().collisionId()});
    auto JProng3ID = trackCollisionMapping.find({B0Candidate.prong3Id(), B0Candidate.prong3_as<aod::Tracks>().collisionId()});
    if (withCollisionAssociator) {
      JProng0ID = trackCollisionMapping.find({B0Candidate.prong0Id(), B0Candidate.collisionId()});
      JProng1ID = trackCollisionMapping.find({B0Candidate.prong1Id(), B0Candidate.collisionId()});
      JProng2ID = trackCollisionMapping.find({B0Candidate.prong2Id(), B0Candidate.collisionId()});
      JProng3ID = trackCollisionMapping.find({B0Candidate.prong3Id(), B0Candidate.collisionId()});
    }
    products.jB0IdsTable(B0Candidate.collisionId(), JProng0ID->second, JProng1ID->second, JProng2ID->second, JProng3ID->second);
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processB0, "produces derived index for B0 candidates", false);

  void processB0MC(aod::HfB0PIds::iterator const& B0Particle)
  {
    products.jB0ParticleIdsTable(B0Particle.mcCollisionId(), B0Particle.mcParticleId());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processB0MC, "produces derived index for B0 particles", false);

  void processBplusCollisions(aod::HfBplusCollIds::iterator const& BplusCollision)
  {
    products.jBplusCollisionIdsTable(BplusCollision.collisionId());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processBplusCollisions, "produces derived index for Bplus collisions", false);

  void processBplusMcCollisions(aod::HfBplusMcCollIds::iterator const& BplusMcCollision)
  {
    products.jBplusMcCollisionIdsTable(BplusMcCollision.mcCollisionId());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processBplusMcCollisions, "produces derived index for Bplus MC collisions", false);

  void processBplus(aod::HfBplusIds::iterator const& BplusCandidate, aod::Tracks const&)
  {
    auto JProng0ID = trackCollisionMapping.find({BplusCandidate.prong0Id(), BplusCandidate.prong0_as<aod::Tracks>().collisionId()});
    auto JProng1ID = trackCollisionMapping.find({BplusCandidate.prong1Id(), BplusCandidate.prong1_as<aod::Tracks>().collisionId()});
    auto JProng2ID = trackCollisionMapping.find({BplusCandidate.prong2Id(), BplusCandidate.prong2_as<aod::Tracks>().collisionId()});
    if (withCollisionAssociator) {
      JProng0ID = trackCollisionMapping.find({BplusCandidate.prong0Id(), BplusCandidate.collisionId()});
      JProng1ID = trackCollisionMapping.find({BplusCandidate.prong1Id(), BplusCandidate.collisionId()});
      JProng2ID = trackCollisionMapping.find({BplusCandidate.prong2Id(), BplusCandidate.collisionId()});
    }
    products.jBplusIdsTable(BplusCandidate.collisionId(), JProng0ID->second, JProng1ID->second, JProng2ID->second);
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processBplus, "produces derived index for Bplus candidates", false);

  void processBplusMC(aod::HfBplusPIds::iterator const& BplusParticle)
  {
    products.jBplusParticleIdsTable(BplusParticle.mcCollisionId(), BplusParticle.mcParticleId());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processBplusMC, "produces derived index for Bplus particles", false);

  void processV0(aod::V0Indices::iterator const& V0Candidate, aod::Tracks const&)
  {
    auto JPosTrackID = trackCollisionMapping.find({V0Candidate.posTrackId(), V0Candidate.posTrack_as<aod::Tracks>().collisionId()});
    auto JNegTrackID = trackCollisionMapping.find({V0Candidate.negTrackId(), V0Candidate.negTrack_as<aod::Tracks>().collisionId()});
    if (withCollisionAssociator) {
      JPosTrackID = trackCollisionMapping.find({V0Candidate.posTrackId(), V0Candidate.collisionId()});
      JNegTrackID = trackCollisionMapping.find({V0Candidate.negTrackId(), V0Candidate.collisionId()});
    }
    products.jV0IdsTable(V0Candidate.collisionId(), JPosTrackID->second, JNegTrackID->second);
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processV0, "produces derived index for V0 candidates", false);

  void processV0MC(aod::McCollision const& mcCollision, aod::McParticles const& particles)
  { // can loop over McV0Labels tables if we want to only store matched V0Particles
    bool filledV0McCollisionTable = false;
    for (auto const& particle : particles) {
      if (jetv0utilities::isV0Particle(particles, particle)) {
        if (!filledV0McCollisionTable) {
          products.jV0McCollisionsTable(mcCollision.posX(), mcCollision.posY(), mcCollision.posZ());
          products.jV0McCollisionIdsTable(mcCollision.globalIndex());
          filledV0McCollisionTable = true;
        }
        std::vector<int32_t> mothersId;
        if (particle.has_mothers()) {
          auto mothersIdTemps = particle.mothersIds();
          for (auto mothersIdTemp : mothersIdTemps) {
            mothersId.push_back(mothersIdTemp);
          }
        }
        int daughtersId[2] = {-1, -1};
        auto i = 0;
        if (particle.has_daughters()) {
          for (auto daughterId : particle.daughtersIds()) {
            if (i > 1) {
              break;
            }
            daughtersId[i] = daughterId;
            i++;
          }
        }
        auto pdgParticle = pdgDatabase->GetParticle(particle.pdgCode());
        products.jV0McsTable(products.jV0McCollisionsTable.lastIndex(), particle.pt(), particle.eta(), particle.phi(), particle.y(), particle.e(), pdgParticle->Mass(), particle.pdgCode(), particle.getGenStatusCode(), particle.getHepMCStatusCode(), particle.isPhysicalPrimary(), jetv0utilities::setV0ParticleDecayBit(particles, particle));
        products.jV0McIdsTable(mcCollision.globalIndex(), particle.globalIndex(), mothersId, daughtersId);
      }
    }
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processV0MC, "produces V0 particles", false);

  void processDielectronCollisions(aod::ReducedEventsInfo::iterator const& DielectronCollision)
  {
    products.jDielectronCollisionIdsTable(DielectronCollision.collisionId());
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processDielectronCollisions, "produces derived index for Dielectron collisions", false);

  void processDielectron(aod::DielectronInfo const& DielectronCandidate, aod::Tracks const&)
  {
    auto JProng0ID = trackCollisionMapping.find({DielectronCandidate.prong0Id(), DielectronCandidate.prong0_as<aod::Tracks>().collisionId()});
    auto JProng1ID = trackCollisionMapping.find({DielectronCandidate.prong1Id(), DielectronCandidate.prong1_as<aod::Tracks>().collisionId()});
    if (withCollisionAssociator) {
      JProng0ID = trackCollisionMapping.find({DielectronCandidate.prong0Id(), DielectronCandidate.collisionId()});
      JProng1ID = trackCollisionMapping.find({DielectronCandidate.prong1Id(), DielectronCandidate.collisionId()});
    }
    products.jDielectronIdsTable(DielectronCandidate.collisionId(), JProng0ID->second, JProng1ID->second);
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processDielectron, "produces derived index for Dielectron candidates", false);

  void processDielectronMc(aod::McCollision const& mcCollision, aod::McParticles const& particles)
  {
    bool filledDielectronMcCollisionTable = false;
    for (auto const& particle : particles) {
      if (jetdqutilities::isDielectronParticle(particles, particle)) {
        if (!filledDielectronMcCollisionTable) {
          products.jDielectronMcCollisionsTable(mcCollision.posX(), mcCollision.posY(), mcCollision.posZ());
          products.jDielectronMcCollisionIdsTable(mcCollision.globalIndex());
          filledDielectronMcCollisionTable = true;
        }
        std::vector<int32_t> mothersId;
        if (particle.has_mothers()) {
          auto mothersIdTemps = particle.mothersIds();
          for (auto mothersIdTemp : mothersIdTemps) {
            mothersId.push_back(mothersIdTemp);
          }
        }
        int daughtersId[2] = {-1, -1};
        auto i = 0;
        if (particle.has_daughters()) {
          for (auto daughterId : particle.daughtersIds()) {
            if (i > 1) {
              break;
            }
            daughtersId[i] = daughterId;
            i++;
          }
        }
        auto pdgParticle = pdgDatabase->GetParticle(particle.pdgCode());
        products.jDielectronMcsTable(products.jDielectronMcCollisionsTable.lastIndex(), particle.pt(), particle.eta(), particle.phi(), particle.y(), particle.e(), pdgParticle->Mass(), particle.pdgCode(), particle.getGenStatusCode(), particle.getHepMCStatusCode(), particle.isPhysicalPrimary(), jetdqutilities::setDielectronParticleDecayBit(particles, particle), RecoDecay::getCharmHadronOrigin(particles, particle, false)); // Todo: should the last thing be false?
        products.jDielectronMcIdsTable(mcCollision.globalIndex(), particle.globalIndex(), mothersId, daughtersId);
        products.JDielectronMcRCollDummysTable(false);
      }
    }
  }
  PROCESS_SWITCH(JetDerivedDataProducerTask, processDielectronMc, "produces Dielectron mccollisions and particles", false);
};

WorkflowSpec defineDataProcessing(ConfigContext const& cfgc)
{
  return WorkflowSpec{
    adaptAnalysisTask<JetDerivedDataProducerTask>(cfgc, TaskName{"jet-deriveddata-producer"})};
}
