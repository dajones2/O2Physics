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

/// \file timeDependentQa.cxx
/// \brief Time-dependent QA for a number of observables
///
/// \author Evgeny Kryshen <evgeny.kryshen@cern.ch> and Igor Altsybeev <Igor.Altsybeev@cern.ch>

#include <map>
#include <vector>
#include <string>

#include "Framework/runDataProcessing.h"
#include "Framework/AnalysisTask.h"
#include "Framework/AnalysisDataModel.h"
#include "Framework/HistogramRegistry.h"
#include "CCDB/BasicCCDBManager.h"
#include "Common/DataModel/EventSelection.h"
#include "Common/DataModel/TrackSelectionTables.h"
#include "Common/CCDB/ctpRateFetcher.h"
#include "TPCCalibration/TPCMShapeCorrection.h"
#include "DataFormatsParameters/GRPECSObject.h"
#include "DataFormatsITSMFT/ROFRecord.h"
#include "ReconstructionDataFormats/Vertex.h"

#include "TTree.h"

using namespace o2;
using namespace o2::framework;
using BCsRun3 = soa::Join<aod::BCs, aod::Timestamps>;
using BarrelTracks = soa::Join<aod::Tracks, aod::TracksExtra, aod::TracksDCA>;
const AxisSpec axisQoverPt{100, -5., 5., "q/p_{T}, 1/GeV"};
const AxisSpec axisDcaR{1000, -5., 5., "DCA_{r}, cm"};
const AxisSpec axisDcaZ{1000, -5., 5., "DCA_{z}, cm"};
const AxisSpec axisSparseQoverPt{20, -5., 5., "q/p_{T}, 1/GeV"};
const AxisSpec axisSparseDcaR{100, -5., 5., "DCA_{r}, cm"};
const AxisSpec axisSparseDcaZ{100, -5., 5., "DCA_{z}, cm"};

struct TimeDependentQaTask {
  Configurable<double> confTimeBinWidthInSec{"TimeBinWidthInSec", 0.25, "Width of time bins in seconds"};                                                                        // o2-linter: disable=name/configurable
  Configurable<int> confTakeVerticesWithUPCsettings{"ConsiderVerticesWithUPCsettings", 0, "Take vertices: 0 - all , 1 - only without UPC settings, 2 - only with UPC settings"}; // o2-linter: disable=name/configurable
  Service<o2::ccdb::BasicCCDBManager> ccdb;
  HistogramRegistry histos{"Histos", {}, OutputObjHandlingPolicy::AnalysisObject};
  o2::tpc::TPCMShapeCorrection mshape; // object for simple access
  int lastRunNumber = -1;
  double maxSec = 1;
  double minSec = 0;
  ctpRateFetcher mRateFetcher;
  void init(InitContext&)
  {
    ccdb->setURL("http://alice-ccdb.cern.ch");
    ccdb->setCaching(true);
    ccdb->setLocalObjectValidityChecking();
    histos.add("hQoverPt", "", kTH1F, {axisQoverPt});
    histos.add("hDcaR", "", kTH1F, {axisDcaR});
    histos.add("hDcaZ", "", kTH1F, {axisDcaZ});
    histos.add("hQoverPtDcaR", "", kTH2F, {axisSparseQoverPt, axisSparseDcaR});
    histos.add("hQoverPtDcaZ", "", kTH2F, {axisSparseQoverPt, axisSparseDcaZ});
  }

  void process(aod::Collision const& col, BCsRun3 const& bcs, BarrelTracks const& tracks)
  {
    int runNumber = bcs.iteratorAt(0).runNumber();
    if (runNumber != lastRunNumber) {
      lastRunNumber = runNumber;
      std::map<std::string, std::string> metadata;
      metadata["runNumber"] = Form("%d", runNumber);
      auto grpecs = ccdb->getSpecific<o2::parameters::GRPECSObject>("GLO/Config/GRPECS", bcs.iteratorAt(0).timestamp(), metadata);
      minSec = floor(grpecs->getTimeStart() / 1000.);
      maxSec = ceil(grpecs->getTimeEnd() / 1000.);
      int nTimeBins = static_cast<int>((maxSec - minSec) / confTimeBinWidthInSec);
      double timeInterval = nTimeBins * confTimeBinWidthInSec;

      const AxisSpec axisSeconds{nTimeBins, 0, timeInterval, "seconds"};
      histos.add("hSecondsCollisions", "", kTH1F, {axisSeconds});

      histos.add("hSecondsAsideQoverPtSumDcaR", "", kTH2F, {axisSeconds, axisSparseQoverPt});
      histos.add("hSecondsAsideQoverPtSumDcaZ", "", kTH2F, {axisSeconds, axisSparseQoverPt});
      histos.add("hSecondsCsideQoverPtSumDcaR", "", kTH2F, {axisSeconds, axisSparseQoverPt});
      histos.add("hSecondsCsideQoverPtSumDcaZ", "", kTH2F, {axisSeconds, axisSparseQoverPt});
      histos.add("hSecondsQoverPtSumDcaR", "", kTH2F, {axisSeconds, axisSparseQoverPt});
      histos.add("hSecondsQoverPtSumDcaZ", "", kTH2F, {axisSeconds, axisSparseQoverPt});

      histos.add("hSecondsAsideSumDcaR", "", kTH1F, {axisSeconds});
      histos.add("hSecondsAsideSumDcaZ", "", kTH1F, {axisSeconds});
      histos.add("hSecondsCsideSumDcaR", "", kTH1F, {axisSeconds});
      histos.add("hSecondsCsideSumDcaZ", "", kTH1F, {axisSeconds});
      histos.add("hSecondsSumDcaR", "", kTH1F, {axisSeconds});
      histos.add("hSecondsSumDcaZ", "", kTH1F, {axisSeconds});
      histos.add("hSecondsTracks", "", kTH1F, {axisSeconds});
      histos.add("hSecondsTracksMshape", "", kTH1F, {axisSeconds});

      histos.add("hSecondsAsideITSTPCcontrib", "", kTH1F, {axisSeconds});
      histos.add("hSecondsCsideITSTPCcontrib", "", kTH1F, {axisSeconds});
      histos.add("hSecondsIR", "", kTH1F, {axisSeconds});

      // QA for UPC settings
      histos.add("hSecondsUPCvertices", "", kTH2F, {axisSeconds, {2, -0.5, 1.5, "Is vertex with UPC settings"}});

      // QA for global tracks
      const AxisSpec axisChi2{40, 0., 20., "chi2/ndof"};
      const AxisSpec axisNclsITS{10, -0.5, 9.5, "n ITS cls"};
      const AxisSpec axisNclsTPC{40, -0.5, 159.5, "n TPC cls"};
      const AxisSpec axisFraction{40, 0, 1., "Fraction shared cls Tpc"};

      histos.add("hSecondsAsideNumTracksGlobal", "", kTH1F, {axisSeconds});
      histos.add("hSecondsAsideSumDcaRglobal", "", kTH1F, {axisSeconds});
      histos.add("hSecondsAsideSumDcaZglobal", "", kTH1F, {axisSeconds});
      histos.add("hSecondsAsideNumClsItsGlobal", "", kTH2F, {axisSeconds, axisNclsITS});
      histos.add("hSecondsAsideChi2NClItsGlobal", "", kTH2F, {axisSeconds, axisChi2});
      histos.add("hSecondsAsideNumClsTpcGlobal", "", kTH2F, {axisSeconds, axisNclsTPC});
      histos.add("hSecondsAsideChi2NClTpcGlobal", "", kTH2F, {axisSeconds, axisChi2});
      histos.add("hSecondsAsideTpcFractionSharedClsGlobal_nTPCclsCut80", "", kTH2F, {axisSeconds, axisFraction});

      histos.add("hSecondsCsideNumTracksGlobal", "", kTH1F, {axisSeconds});
      histos.add("hSecondsCsideSumDcaRglobal", "", kTH1F, {axisSeconds});
      histos.add("hSecondsCsideSumDcaZglobal", "", kTH1F, {axisSeconds});
      histos.add("hSecondsCsideNumClsItsGlobal", "", kTH2F, {axisSeconds, axisNclsITS});
      histos.add("hSecondsCsideChi2NClItsGlobal", "", kTH2F, {axisSeconds, axisChi2});
      histos.add("hSecondsCsideNumClsTpcGlobal", "", kTH2F, {axisSeconds, axisNclsTPC});
      histos.add("hSecondsCsideChi2NClTpcGlobal", "", kTH2F, {axisSeconds, axisChi2});
      histos.add("hSecondsCsideTpcFractionSharedClsGlobal_nTPCclsCut80", "", kTH2F, {axisSeconds, axisFraction});

      const AxisSpec axisPhi{64, 0, TMath::TwoPi(), "#varphi"}; // o2-linter: disable=external-pi
      const AxisSpec axisEta{10, -0.8, 0.8, "#eta"};
      histos.add("hSecondsITSlayer0vsPhi", "", kTH2F, {axisSeconds, axisPhi});
      histos.add("hSecondsITSlayer1vsPhi", "", kTH2F, {axisSeconds, axisPhi});
      histos.add("hSecondsITSlayer2vsPhi", "", kTH2F, {axisSeconds, axisPhi});
      histos.add("hSecondsITSlayer3vsPhi", "", kTH2F, {axisSeconds, axisPhi});
      histos.add("hSecondsITSlayer4vsPhi", "", kTH2F, {axisSeconds, axisPhi});
      histos.add("hSecondsITSlayer5vsPhi", "", kTH2F, {axisSeconds, axisPhi});
      histos.add("hSecondsITSlayer6vsPhi", "", kTH2F, {axisSeconds, axisPhi});
      histos.add("hSecondsITS7clsVsPhi", "", kTH2F, {axisSeconds, axisPhi});
      histos.add("hSecondsITSglobalVsPhi", "", kTH2F, {axisSeconds, axisPhi});
      histos.add("hSecondsITSTRDVsPhi", "", kTH2F, {axisSeconds, axisPhi});
      histos.add("hSecondsITSTOFVsPhi", "", kTH2F, {axisSeconds, axisPhi});
      histos.add("hSecondsITSglobalVsEtaPhi", "", kTH3F, {axisSeconds, axisEta, axisPhi});
    }

    auto bc = col.bc_as<BCsRun3>();
    int64_t ts = bc.timestamp();
    double secFromSOR = ts / 1000. - minSec;

    // check if a vertex is found in the UPC mode ITS ROF, flags from: https://github.com/AliceO2Group/AliceO2/blob/dev/DataFormats/Reconstruction/include/ReconstructionDataFormats/Vertex.h
    ushort flags = col.flags();
    bool isVertexUPC = flags & dataformats::Vertex<o2::dataformats::TimeStamp<int>>::Flags::UPCMode; // is vertex with UPC settings
    histos.fill(HIST("hSecondsUPCvertices"), secFromSOR, isVertexUPC ? 1 : 0);

    if (confTakeVerticesWithUPCsettings > 0) {
      if (confTakeVerticesWithUPCsettings == 1 && isVertexUPC) // reject vertices with UPC settings
        return;
      if (confTakeVerticesWithUPCsettings == 2 && !isVertexUPC) // we want to select vertices with UPC settings --> reject vertices reconstructed with "normal" settings
        return;
      // LOGP(info, "flags={} nTracks = {}", flags, tracks.size());
    }

    histos.fill(HIST("hSecondsCollisions"), secFromSOR);

    double hadronicRate = mRateFetcher.fetch(ccdb.service, ts, runNumber, "ZNC hadronic") * 1.e-3; //
    histos.fill(HIST("hSecondsIR"), secFromSOR, hadronicRate);

    // checking mShape flags in time:
    auto mShapeTree = ccdb->getForTimeStamp<TTree>("TPC/Calib/MShapePotential", ts);
    mshape.setFromTree(*mShapeTree);
    bool isMshape = !mshape.getBoundaryPotential(ts).mPotential.empty();

    int nAsideITSTPCContrib = 0;
    int nCsideITSTPCContrib = 0;
    for (const auto& track : tracks) {
      if (!track.hasTPC() || !track.hasITS()) {
        continue;
      }

      float qpt = track.signed1Pt();
      float dcaR = track.dcaXY();
      float dcaZ = track.dcaZ();

      LOGP(debug, "dcaR = {} dcaZ = {}", dcaR, dcaZ);
      histos.fill(HIST("hQoverPt"), qpt);
      histos.fill(HIST("hDcaR"), dcaR);
      histos.fill(HIST("hDcaZ"), dcaZ);
      histos.fill(HIST("hQoverPtDcaR"), qpt, dcaR);
      histos.fill(HIST("hQoverPtDcaZ"), qpt, dcaZ);
      histos.fill(HIST("hSecondsSumDcaR"), secFromSOR, dcaR);
      histos.fill(HIST("hSecondsSumDcaZ"), secFromSOR, dcaZ);
      histos.fill(HIST("hSecondsQoverPtSumDcaR"), secFromSOR, qpt, dcaR);
      histos.fill(HIST("hSecondsQoverPtSumDcaZ"), secFromSOR, qpt, dcaZ);

      histos.fill(HIST("hSecondsTracks"), secFromSOR);

      if (track.tgl() > 0.) {
        histos.fill(HIST("hSecondsAsideQoverPtSumDcaR"), secFromSOR, qpt, dcaR);
        histos.fill(HIST("hSecondsAsideQoverPtSumDcaZ"), secFromSOR, qpt, dcaZ);
        histos.fill(HIST("hSecondsAsideSumDcaR"), secFromSOR, dcaR);
        histos.fill(HIST("hSecondsAsideSumDcaZ"), secFromSOR, dcaZ);

      } else {
        histos.fill(HIST("hSecondsCsideQoverPtSumDcaR"), secFromSOR, qpt, dcaR);
        histos.fill(HIST("hSecondsCsideQoverPtSumDcaZ"), secFromSOR, qpt, dcaZ);
        histos.fill(HIST("hSecondsCsideSumDcaR"), secFromSOR, dcaR);
        histos.fill(HIST("hSecondsCsideSumDcaZ"), secFromSOR, dcaZ);
      }
      if (isMshape) {
        histos.fill(HIST("hSecondsTracksMshape"), secFromSOR);
      }

      if (track.hasTPC() && track.hasITS() && std::fabs(track.eta()) < 0.8 && std::fabs(track.pt()) > 0.2) {
        if (track.tgl() > 0.) {
          histos.fill(HIST("hSecondsAsideNumTracksGlobal"), secFromSOR);
          histos.fill(HIST("hSecondsAsideSumDcaRglobal"), secFromSOR, dcaR);
          histos.fill(HIST("hSecondsAsideSumDcaZglobal"), secFromSOR, dcaZ);
          histos.fill(HIST("hSecondsAsideNumClsItsGlobal"), secFromSOR, track.itsNCls());
          histos.fill(HIST("hSecondsAsideChi2NClItsGlobal"), secFromSOR, track.itsChi2NCl());
          histos.fill(HIST("hSecondsAsideNumClsTpcGlobal"), secFromSOR, track.tpcNClsFound());
          histos.fill(HIST("hSecondsAsideChi2NClTpcGlobal"), secFromSOR, track.tpcChi2NCl());
          if (track.tpcNClsFound() >= 80)
            histos.fill(HIST("hSecondsAsideTpcFractionSharedClsGlobal_nTPCclsCut80"), secFromSOR, track.tpcFractionSharedCls());
        } else {
          histos.fill(HIST("hSecondsCsideNumTracksGlobal"), secFromSOR);
          histos.fill(HIST("hSecondsCsideSumDcaRglobal"), secFromSOR, dcaR);
          histos.fill(HIST("hSecondsCsideSumDcaZglobal"), secFromSOR, dcaZ);
          histos.fill(HIST("hSecondsCsideNumClsItsGlobal"), secFromSOR, track.itsNCls());
          histos.fill(HIST("hSecondsCsideChi2NClItsGlobal"), secFromSOR, track.itsChi2NCl());
          histos.fill(HIST("hSecondsCsideNumClsTpcGlobal"), secFromSOR, track.tpcNClsFound());
          histos.fill(HIST("hSecondsCsideChi2NClTpcGlobal"), secFromSOR, track.tpcChi2NCl());
          if (track.tpcNClsFound() >= 80)
            histos.fill(HIST("hSecondsCsideTpcFractionSharedClsGlobal_nTPCclsCut80"), secFromSOR, track.tpcFractionSharedCls());
        }
      } // end of ITS+TPC tracks

      if (track.isPVContributor()) {
        if (track.tgl() > 0.) {
          nAsideITSTPCContrib++;
        } else {
          nCsideITSTPCContrib++;
        }

        // select straight tracks
        if (track.pt() < 1) {
          continue;
        }
        // study ITS cluster pattern vs sec
        if (track.itsClusterMap() & (1 << 0))
          histos.fill(HIST("hSecondsITSlayer0vsPhi"), secFromSOR, track.phi());
        if (track.itsClusterMap() & (1 << 1))
          histos.fill(HIST("hSecondsITSlayer1vsPhi"), secFromSOR, track.phi());
        if (track.itsClusterMap() & (1 << 2))
          histos.fill(HIST("hSecondsITSlayer2vsPhi"), secFromSOR, track.phi());
        if (track.itsClusterMap() & (1 << 3))
          histos.fill(HIST("hSecondsITSlayer3vsPhi"), secFromSOR, track.phi());
        if (track.itsClusterMap() & (1 << 4))
          histos.fill(HIST("hSecondsITSlayer4vsPhi"), secFromSOR, track.phi());
        if (track.itsClusterMap() & (1 << 5))
          histos.fill(HIST("hSecondsITSlayer5vsPhi"), secFromSOR, track.phi());
        if (track.itsClusterMap() & (1 << 6))
          histos.fill(HIST("hSecondsITSlayer6vsPhi"), secFromSOR, track.phi());
        if (track.itsNCls() == 7)
          histos.fill(HIST("hSecondsITS7clsVsPhi"), secFromSOR, track.phi());
        if (track.hasITS() && track.hasTPC()) {
          histos.fill(HIST("hSecondsITSglobalVsPhi"), secFromSOR, track.phi());
          histos.fill(HIST("hSecondsITSglobalVsEtaPhi"), secFromSOR, track.eta(), track.phi());
        }
        if (track.hasTRD())
          histos.fill(HIST("hSecondsITSTRDVsPhi"), secFromSOR, track.phi());
        if (track.hasTOF())
          histos.fill(HIST("hSecondsITSTOFVsPhi"), secFromSOR, track.phi());
      }
    }
    histos.fill(HIST("hSecondsAsideITSTPCcontrib"), secFromSOR, nAsideITSTPCContrib);
    histos.fill(HIST("hSecondsCsideITSTPCcontrib"), secFromSOR, nCsideITSTPCContrib);
  }
};

WorkflowSpec defineDataProcessing(ConfigContext const& cfgc)
{
  return WorkflowSpec{
    adaptAnalysisTask<TimeDependentQaTask>(cfgc)};
}
