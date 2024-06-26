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
///
/// \author Alberto Caliva (alberto.caliva@cern.ch)
/// \since June 27, 2023

#include "Framework/runDataProcessing.h"
#include "Framework/AnalysisTask.h"
#include "Framework/AnalysisDataModel.h"
#include "Framework/ASoA.h"
#include "Framework/ASoAHelpers.h"
#include "Framework/HistogramRegistry.h"
#include "Common/Core/TrackSelection.h"
#include "Common/Core/trackUtilities.h"
#include "Common/DataModel/EventSelection.h"
#include "Common/DataModel/PIDResponse.h"
#include "Common/DataModel/TrackSelectionTables.h"
#include "Common/Core/PID/PIDTOF.h"
#include "Common/TableProducer/PID/pidTOFBase.h"
#include "ReconstructionDataFormats/Track.h"
#include "ReconstructionDataFormats/PID.h"
#include "ReconstructionDataFormats/TrackParametrization.h"
#include "ReconstructionDataFormats/DCA.h"
#include "PWGLF/DataModel/LFParticleIdentification.h"

using namespace o2;
using namespace o2::framework;
using namespace o2::framework::expressions;
using namespace o2::constants::physics;

struct AntimatterAbsorptionHMPID {

  // Histograms
  HistogramRegistry registryQC{"registryQC", {}, OutputObjHandlingPolicy::AnalysisObject, true, true};
  HistogramRegistry registryDA{"registryDA", {}, OutputObjHandlingPolicy::AnalysisObject, true, true};

  // Track Selection Parameters
  Configurable<float> pmin{"pmin", 0.1, "pmin"};
  Configurable<float> pmax{"pmax", 3.0, "pmax"};
  Configurable<float> etaMin{"etaMin", -0.8, "etaMin"};
  Configurable<float> etaMax{"etaMax", +0.8, "etaMax"};
  Configurable<float> phiMin{"phiMin", 0.0, "phiMin"};
  Configurable<float> phiMax{"phiMax", 2.0 * TMath::Pi(), "phiMax"};
  Configurable<float> nsigmaTPCMin{"nsigmaTPCMin", -3.0, "nsigmaTPCMin"};
  Configurable<float> nsigmaTPCMax{"nsigmaTPCMax", +3.0, "nsigmaTPCMax"};
  Configurable<float> nsigmaTOFMin{"nsigmaTOFMin", -3.0, "nsigmaTOFMin"};
  Configurable<float> nsigmaTOFMax{"nsigmaTOFMax", +3.5, "nsigmaTOFMax"};
  Configurable<float> minReqClusterITS{"minReqClusterITS", 4.0, "min number of clusters required in ITS"};
  Configurable<float> minTPCnClsFound{"minTPCnClsFound", 50.0f, "minTPCnClsFound"};
  Configurable<float> minNCrossedRowsTPC{"minNCrossedRowsTPC", 70.0f, "min number of crossed rows TPC"};
  Configurable<float> maxChi2ITS{"maxChi2ITS", 36.0f, "max chi2 per cluster ITS"};
  Configurable<float> maxChi2TPC{"maxChi2TPC", 4.0f, "max chi2 per cluster TPC"};
  Configurable<float> maxDCAxy{"maxDCAxy", 0.5f, "maxDCAxy"};
  Configurable<float> maxDCAz{"maxDCAz", 0.5f, "maxDCAz"};

  void init(o2::framework::InitContext&)
  {
    // Event Counter
    registryQC.add("number_of_events_data", "number of events in data", HistType::kTH1F, {{10, 0, 10, "counter"}});

    // HMPID Maps
    registryDA.add("hmpidXYpos", "hmpidXYpos", HistType::kTH2F, {{300, 0.0, 300.0, "x_{MIP}"}, {300, 0.0, 300.0, "y_{MIP}"}});
    registryDA.add("hmpidXYneg", "hmpidXYneg", HistType::kTH2F, {{300, 0.0, 300.0, "x_{MIP}"}, {300, 0.0, 300.0, "y_{MIP}"}});

    // Pion Pos
    registryDA.add("incomingPi_Pos_8cm", "incomingPi_Pos_8cm", HistType::kTH1F, {{290, 0.1, 3.0, "#it{p} (GeV/#it{c})"}});
    registryDA.add("incomingPi_Pos_4cm", "incomingPi_Pos_4cm", HistType::kTH1F, {{290, 0.1, 3.0, "#it{p} (GeV/#it{c})"}});
    registryDA.add("survivingPi_Pos_8cm", "survivingPi_Pos_8cm", HistType::kTH2F, {{290, 0.1, 3.0, "#it{p} (GeV/#it{c})"}, {300, 0.0, 30.0, "#Delta R (cm)"}});
    registryDA.add("survivingPi_Pos_4cm", "survivingPi_Pos_4cm", HistType::kTH2F, {{290, 0.1, 3.0, "#it{p} (GeV/#it{c})"}, {300, 0.0, 30.0, "#Delta R (cm)"}});
    registryDA.add("Pi_Pos_Q_8cm", "Pi_Pos_Q_8cm", HistType::kTH2F, {{290, 0.1, 3.0, "#it{p} (GeV/#it{c})"}, {200, 0.0, 2000.0, "Q (ADC)"}});
    registryDA.add("Pi_Pos_Q_4cm", "Pi_Pos_Q_4cm", HistType::kTH2F, {{290, 0.1, 3.0, "#it{p} (GeV/#it{c})"}, {200, 0.0, 2000.0, "Q (ADC)"}});
    registryDA.add("Pi_Pos_ClsSize_8cm", "Pi_Pos_ClsSize_8cm", HistType::kTH2F, {{290, 0.1, 3.0, "#it{p} (GeV/#it{c})"}, {200, 0.0, 2000.0, "Cls size"}});
    registryDA.add("Pi_Pos_ClsSize_4cm", "Pi_Pos_ClsSize_4cm", HistType::kTH2F, {{290, 0.1, 3.0, "#it{p} (GeV/#it{c})"}, {200, 0.0, 2000.0, "Cls size"}});
    registryDA.add("Pi_Pos_momentum", "Pi_Pos_momentum", HistType::kTH2F, {{100, 0.0, 3.0, "#it{p}_{vtx} (GeV/#it{c})"}, {100, 0.0, 3.0, "#it{p}_{mhpid} (GeV/#it{c})"}});

    // Pion Neg
    registryDA.add("incomingPi_Neg_8cm", "incomingPi_Neg_8cm", HistType::kTH1F, {{290, 0.1, 3.0, "#it{p} (GeV/#it{c})"}});
    registryDA.add("incomingPi_Neg_4cm", "incomingPi_Neg_4cm", HistType::kTH1F, {{290, 0.1, 3.0, "#it{p} (GeV/#it{c})"}});
    registryDA.add("survivingPi_Neg_8cm", "survivingPi_Neg_8cm", HistType::kTH2F, {{290, 0.1, 3.0, "#it{p} (GeV/#it{c})"}, {300, 0.0, 30.0, "#Delta R (cm)"}});
    registryDA.add("survivingPi_Neg_4cm", "survivingPi_Neg_4cm", HistType::kTH2F, {{290, 0.1, 3.0, "#it{p} (GeV/#it{c})"}, {300, 0.0, 30.0, "#Delta R (cm)"}});
    registryDA.add("Pi_Neg_Q_8cm", "Pi_Neg_Q_8cm", HistType::kTH2F, {{290, 0.1, 3.0, "#it{p} (GeV/#it{c})"}, {200, 0.0, 2000.0, "Q (ADC)"}});
    registryDA.add("Pi_Neg_Q_4cm", "Pi_Neg_Q_4cm", HistType::kTH2F, {{290, 0.1, 3.0, "#it{p} (GeV/#it{c})"}, {200, 0.0, 2000.0, "Q (ADC)"}});
    registryDA.add("Pi_Neg_ClsSize_8cm", "Pi_Neg_ClsSize_8cm", HistType::kTH2F, {{290, 0.1, 3.0, "#it{p} (GeV/#it{c})"}, {200, 0.0, 2000.0, "Cls size"}});
    registryDA.add("Pi_Neg_ClsSize_4cm", "Pi_Neg_ClsSize_4cm", HistType::kTH2F, {{290, 0.1, 3.0, "#it{p} (GeV/#it{c})"}, {200, 0.0, 2000.0, "Cls size"}});
    registryDA.add("Pi_Neg_momentum", "Pi_Neg_momentum", HistType::kTH2F, {{100, 0.0, 3.0, "#it{p}_{vtx} (GeV/#it{c})"}, {100, 0.0, 3.0, "#it{p}_{mhpid} (GeV/#it{c})"}});
  }

  // Single-Track Selection
  template <typename trackType>
  bool passedTrackSelection(const trackType& track)
  {
    if (!track.hasITS())
      return false;
    if (!track.hasTPC())
      return false;
    if (!track.hasTOF())
      return false;
    // if (!track.has_hmpid())
    // return false;
    if (!track.passedITSRefit())
      return false;
    if (!track.passedTPCRefit())
      return false;
    if (track.itsNCls() < minReqClusterITS)
      return false;
    if (track.tpcNClsFound() < minTPCnClsFound)
      return false;
    if (track.tpcNClsCrossedRows() < minNCrossedRowsTPC)
      return false;
    if (track.tpcChi2NCl() > maxChi2TPC)
      return false;
    if (track.itsChi2NCl() > maxChi2ITS)
      return false;
    if (TMath::Abs(track.dcaXY()) > maxDCAxy)
      return false;
    if (TMath::Abs(track.dcaZ()) > maxDCAz)
      return false;
    /*
      if (track.eta() < etaMin)
      return false;
    if (track.eta() > etaMax)
      return false;
    if (track.phi() < phiMin)
      return false;
    if (track.phi() > phiMax)
      return false;*/

    return true;
  }

  // Particle Identification (Pions)
  template <typename pionCandidate>
  bool passedPionSelection(const pionCandidate& track)
  {
    if (track.tpcNSigmaPi() < nsigmaTPCMin)
      return false;
    if (track.tpcNSigmaPi() > nsigmaTPCMax)
      return false;
    if (track.tofNSigmaPi() < nsigmaTOFMin)
      return false;
    if (track.tofNSigmaPi() > nsigmaTOFMax)
      return false;

    return true;
  }

  // Info for TPC PID
  using PidInfoTPC = soa::Join<aod::pidTPCLfFullPi, aod::pidTPCLfFullKa,
                               aod::pidTPCLfFullPr, aod::pidTPCLfFullDe,
                               aod::pidTPCLfFullTr, aod::pidTPCLfFullHe,
                               aod::pidTPCLfFullAl>;

  // Info for TOF PID
  using PidInfoTOF = soa::Join<aod::pidTOFFullPi, aod::pidTOFFullKa,
                               aod::pidTOFFullPr, aod::pidTOFFullDe,
                               aod::pidTOFFullTr, aod::pidTOFFullHe,
                               aod::pidTOFFullAl,
                               aod::TOFSignal, aod::pidTOFmass, aod::pidTOFbeta>;

  // Full Tracks
  using FullTracks = soa::Join<aod::Tracks, aod::TracksExtra, aod::TracksDCA, aod::TrackSelection, aod::TrackSelectionExtension, PidInfoTPC, PidInfoTOF>;

  // Process Data
  void processData(o2::soa::Join<o2::aod::Collisions, o2::aod::EvSels>::iterator const& event,
                   FullTracks const& tracks,
                   o2::aod::HMPIDs const& hmpids)
  {
    // Event Selection
    registryQC.fill(HIST("number_of_events_data"), 0.5);

    if (!event.sel8())
      return;

    // Event Counter
    registryQC.fill(HIST("number_of_events_data"), 1.5);

    for (const auto& hmpid : hmpids) {

      // Get Track
      const auto& track = hmpid.track_as<FullTracks>();

      // Track Selection
      if (!passedTrackSelection(track))
        continue;

      if (track.sign() > 0) {
        registryDA.fill(HIST("hmpidXYpos"), hmpid.hmpidXMip(), hmpid.hmpidYMip());
      }
      if (track.sign() < 0) {
        registryDA.fill(HIST("hmpidXYneg"), hmpid.hmpidXMip(), hmpid.hmpidYMip());
      }

      // Particle Identification
      bool passedPionSel = false;
      if (passedPionSelection(track))
        passedPionSel = true;

      // Absorber
      bool hmpidAbs8cm = true;
      bool hmpidAbs4cm = true;

      // Distance between extrapolated and matched point
      float dx = hmpid.hmpidXTrack() - hmpid.hmpidXMip();
      float dy = hmpid.hmpidYTrack() - hmpid.hmpidYMip();
      float dr = sqrt(dx * dx + dy * dy);

      // Fill Histograms for Positive Pions
      if (passedPionSel && track.sign() > 0) {

        if (hmpidAbs8cm) {
          registryDA.fill(HIST("incomingPi_Pos_8cm"), hmpid.hmpidMom());
          registryDA.fill(HIST("survivingPi_Pos_8cm"), hmpid.hmpidMom(), dr);
          registryDA.fill(HIST("Pi_Pos_Q_8cm"), hmpid.hmpidMom(), hmpid.hmpidQMip());
          registryDA.fill(HIST("Pi_Pos_ClsSize_8cm"), hmpid.hmpidMom(), hmpid.hmpidClusSize());
        }
        if (hmpidAbs4cm) {
          registryDA.fill(HIST("incomingPi_Pos_4cm"), hmpid.hmpidMom());
          registryDA.fill(HIST("survivingPi_Pos_4cm"), hmpid.hmpidMom(), dr);
          registryDA.fill(HIST("Pi_Pos_Q_4cm"), hmpid.hmpidMom(), hmpid.hmpidQMip());
          registryDA.fill(HIST("Pi_Pos_ClsSize_4cm"), hmpid.hmpidMom(), hmpid.hmpidClusSize());
        }
      }

      // Fill Histograms for Negative Pions
      if (passedPionSel && track.sign() < 0) {

        if (hmpidAbs8cm) {
          registryDA.fill(HIST("incomingPi_Neg_8cm"), hmpid.hmpidMom());
          registryDA.fill(HIST("survivingPi_Neg_8cm"), hmpid.hmpidMom(), dr);
          registryDA.fill(HIST("Pi_Neg_Q_8cm"), hmpid.hmpidMom(), hmpid.hmpidQMip());
          registryDA.fill(HIST("Pi_Neg_ClsSize_8cm"), hmpid.hmpidMom(), hmpid.hmpidClusSize());
        }
        if (hmpidAbs4cm) {
          registryDA.fill(HIST("incomingPi_Neg_4cm"), hmpid.hmpidMom());
          registryDA.fill(HIST("survivingPi_Neg_4cm"), hmpid.hmpidMom(), dr);
          registryDA.fill(HIST("Pi_Neg_Q_4cm"), hmpid.hmpidMom(), hmpid.hmpidQMip());
          registryDA.fill(HIST("Pi_Neg_ClsSize_4cm"), hmpid.hmpidMom(), hmpid.hmpidClusSize());
        }
      }
    }
  }
  PROCESS_SWITCH(AntimatterAbsorptionHMPID, processData, "process data", true);
};

//*************************************************************************************************************************************

WorkflowSpec defineDataProcessing(ConfigContext const& cfgc)
{
  return WorkflowSpec{adaptAnalysisTask<AntimatterAbsorptionHMPID>(cfgc)};
}
