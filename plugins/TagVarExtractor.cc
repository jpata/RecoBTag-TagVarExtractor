// -*- C++ -*-
//
// Package:    TagVarExtractor
// Class:      TagVarExtractor
// 
/**\class TagVarExtractor TagVarExtractor.cc RecoBTag/TagVarExtractor/plugins/TagVarExtractor.cc

 Description: [one line class summary]

 Implementation:
     [Notes on implementation]
*/
//
// Original Author:  Dinko Ferencek
//         Created:  Sun Jul 13 18:01:39 EDT 2014
// $Id$
//
//

#define NORMALIZE(x) if(x<-1000 || x!=x) {x = -10;};

// system include files
#include <memory>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"

#include "DataFormats/Math/interface/deltaR.h"

#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "RecoBTag/PerformanceMeasurements/interface/JetInfoBranches.h"
#include "RecoBTag/PerformanceMeasurements/interface/EventInfoBranches.h"

#include "RecoBTag/TagVarExtractor/interface/TagVarBranches.h"

#include <TString.h>
#include <TChain.h>
#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TProfile.h>
#include <TLorentzVector.h>
#include <TF1.h>

#include <string>
#include <map>
#include <vector>
//
// class declaration
//

class TagVarExtractor : public edm::EDAnalyzer {
   public:
      explicit TagVarExtractor(const edm::ParameterSet&);
      ~TagVarExtractor();

      static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);


   private:
      virtual void beginJob() ;
      virtual void analyze(const edm::Event&, const edm::EventSetup&);
      virtual void endJob() ;

      virtual void beginRun(edm::Run const&, edm::EventSetup const&);
      virtual void endRun(edm::Run const&, edm::EventSetup const&);
      virtual void beginLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&);
      virtual void endLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&);

      // ----------member data ---------------------------

      // input branches
      EventInfoBranches EvtInfo;
      JetInfoBranches   JetInfo;

      // input trees
      TChain* JetTree;

      // output branches
      TagVarBranches TagVarInfo;
      // output tree
      TTree *TagVarTree;
      // TFileService for output file
      edm::Service<TFileService> fs;

      // configurables
      const int                       maxEvents_;
      const int                       reportEvery_;
      const std::string               inputTTree_;
      const std::string               varPrefix_;
      const std::vector<std::string>  inputFiles_;
      const double                    jetPtMin_;
      const double                    jetPtMax_;
      const double                    jetAbsEtaMin_;
      const double                    jetAbsEtaMax_;
};

//
// constants, enums and typedefs
//

//
// static data member definitions
//

//
// constructors and destructor
//
TagVarExtractor::TagVarExtractor(const edm::ParameterSet& iConfig) :

  maxEvents_(iConfig.getParameter<int>("MaxEvents")),
  reportEvery_(iConfig.getParameter<int>("ReportEvery")),
  inputTTree_(iConfig.getParameter<std::string>("InputTTree")),
  varPrefix_(iConfig.getParameter<std::string>("VarPrefix")),
  inputFiles_(iConfig.getParameter<std::vector<std::string> >("InputFiles")),
  jetPtMin_(iConfig.getParameter<double>("JetPtMin")),
  jetPtMax_(iConfig.getParameter<double>("JetPtMax")),
  jetAbsEtaMin_(iConfig.getParameter<double>("JetAbsEtaMin")),
  jetAbsEtaMax_(iConfig.getParameter<double>("JetAbsEtaMax"))

{
   //now do what ever initialization is needed

   // create output tree
   TagVarTree = fs->make<TTree>("ttree", "ttree");
   TagVarInfo.doTagVarsCSV = iConfig.getParameter<bool>("doTagVarsCSV");
   // register output branches
   TagVarInfo.RegisterTree(TagVarTree);
}


TagVarExtractor::~TagVarExtractor()
{
 
   // do anything here that needs to be done at desctruction time
   // (e.g. close files, deallocate resources etc.)

}


//
// member functions
//
// ------------ method called once each job just before starting event loop  ------------
void 
TagVarExtractor::beginJob()
{
  // add input trees
  JetTree = new TChain(inputTTree_.c_str());

  for(unsigned i=0; i<inputFiles_.size(); ++i)
    JetTree->Add(inputFiles_.at(i).c_str());

  // read input branches
  EvtInfo.ReadTree(JetTree);
  JetInfo.ReadTree(JetTree,varPrefix_);
  JetInfo.ReadCSVTagVarTree(JetTree,varPrefix_);
}

// ------------ method called for each event  ------------
void
TagVarExtractor::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{
  if(JetTree == 0) return;

  Long64_t nEntries = JetTree->GetEntries();
  if(maxEvents_>=0) nEntries = ( nEntries>=maxEvents_ ? maxEvents_ : nEntries );

  //---------------------------- Start event loop ---------------------------------------//
  for(Long64_t iEntry = 0; iEntry < nEntries; ++iEntry)
  {
    JetTree->GetEntry(iEntry);
    if((iEntry%reportEvery_) == 0) edm::LogInfo("EventNumber") << "Processing event " << iEntry << " of " << nEntries;

    if(JetInfo.nJet <= 0) continue; // require at least 1 fat jet in the event

    //---------------------------- Start fat jet loop ---------------------------------------//
    for(int iJet = 0; iJet < JetInfo.nJet; ++iJet)
    {
      if ( JetInfo.Jet_pt[iJet] < jetPtMin_ ||
           JetInfo.Jet_pt[iJet] > jetPtMax_ ) continue;                  // apply jet pT cut
      if ( fabs(JetInfo.Jet_eta[iJet]) < fabs(jetAbsEtaMin_) ||
           fabs(JetInfo.Jet_eta[iJet]) > fabs(jetAbsEtaMax_) ) continue; // apply jet eta cut

      memset(&TagVarInfo,0x00,sizeof(TagVarInfo));

      TagVarInfo.Jet_pt        = JetInfo.Jet_pt[iJet];
      TagVarInfo.Jet_eta       = JetInfo.Jet_eta[iJet];
      TagVarInfo.Jet_phi       = JetInfo.Jet_phi[iJet];
      TagVarInfo.Jet_mass      = JetInfo.Jet_mass[iJet];
      TagVarInfo.Jet_flavour   = JetInfo.Jet_flavour[iJet];
      TagVarInfo.Jet_nbHadrons = JetInfo.Jet_nbHadrons[iJet];
      TagVarInfo.Jet_JP        = JetInfo.Jet_Proba[iJet];
      TagVarInfo.Jet_JBP       = JetInfo.Jet_Bprob[iJet];
      TagVarInfo.Jet_CSV       = JetInfo.Jet_CombSvx[iJet];
      TagVarInfo.Jet_CSVIVF    = JetInfo.Jet_CombIVF[iJet];
      TagVarInfo.Jet_CombMVA   = JetInfo.Jet_CombMVA[iJet];
      TagVarInfo.Jet_CombMVAV2   = JetInfo.Jet_CombMVAV2[iJet];
      TagVarInfo.Jet_SoftEl    = JetInfo.Jet_SoftEl[iJet];
      TagVarInfo.Jet_SoftMu    = JetInfo.Jet_SoftMu[iJet];
      TagVarInfo.Jet_Weight    = 1.0;
  
      NORMALIZE(TagVarInfo.Jet_CSV);
      NORMALIZE(TagVarInfo.Jet_CSVIVF);
      NORMALIZE(TagVarInfo.Jet_CombMVA);
      NORMALIZE(TagVarInfo.Jet_CombMVAV2);
      NORMALIZE(TagVarInfo.Jet_SoftEl);
      NORMALIZE(TagVarInfo.Jet_SoftMu);

      TagVarInfo.TagVarCSV_jetNTracks              = JetInfo.TagVarCSV_jetNTracks[iJet];
      TagVarInfo.TagVarCSV_jetNTracksEtaRel        = JetInfo.TagVarCSV_jetNTracksEtaRel[iJet];
      TagVarInfo.TagVarCSV_trackSumJetEtRatio      = (JetInfo.TagVarCSV_trackSumJetEtRatio[iJet] < -1000. ? -1. : JetInfo.TagVarCSV_trackSumJetEtRatio[iJet]);
      TagVarInfo.TagVarCSV_trackSumJetDeltaR       = (JetInfo.TagVarCSV_trackSumJetDeltaR[iJet] < -1000. ? -1. : JetInfo.TagVarCSV_trackSumJetDeltaR[iJet]);
      TagVarInfo.TagVarCSV_trackSip2dValAboveCharm = (JetInfo.TagVarCSV_trackSip2dValAboveCharm[iJet] < -1000. ? -99. : JetInfo.TagVarCSV_trackSip2dValAboveCharm[iJet]);
      TagVarInfo.TagVarCSV_trackSip2dSigAboveCharm = (JetInfo.TagVarCSV_trackSip2dSigAboveCharm[iJet] < -1000. ? -99. : JetInfo.TagVarCSV_trackSip2dSigAboveCharm[iJet]);
      TagVarInfo.TagVarCSV_trackSip3dValAboveCharm = (JetInfo.TagVarCSV_trackSip3dValAboveCharm[iJet] < -1000. ? -99. : JetInfo.TagVarCSV_trackSip3dValAboveCharm[iJet]);
      TagVarInfo.TagVarCSV_trackSip3dSigAboveCharm = (JetInfo.TagVarCSV_trackSip3dSigAboveCharm[iJet] < -1000. ? -99. : JetInfo.TagVarCSV_trackSip3dSigAboveCharm[iJet]);
      TagVarInfo.TagVarCSV_vertexCategory          = (JetInfo.TagVarCSV_vertexCategory[iJet] < -1000. ? -1. : JetInfo.TagVarCSV_vertexCategory[iJet]);
      TagVarInfo.TagVarCSV_jetNSecondaryVertices   = JetInfo.TagVarCSV_jetNSecondaryVertices[iJet];
      TagVarInfo.TagVarCSV_vertexMass              = (JetInfo.TagVarCSV_vertexMass[iJet] < -1000. ? -1. : JetInfo.TagVarCSV_vertexMass[iJet]);
      TagVarInfo.TagVarCSV_vertexNTracks           = JetInfo.TagVarCSV_vertexNTracks[iJet];
      TagVarInfo.TagVarCSV_vertexEnergyRatio       = (JetInfo.TagVarCSV_vertexEnergyRatio[iJet] < -1000. ? -1. : JetInfo.TagVarCSV_vertexEnergyRatio[iJet]);
      TagVarInfo.TagVarCSV_vertexJetDeltaR         = (JetInfo.TagVarCSV_vertexJetDeltaR[iJet] < -1000. ? -1. : JetInfo.TagVarCSV_vertexJetDeltaR[iJet]);
      TagVarInfo.TagVarCSV_flightDistance2dVal     = (JetInfo.TagVarCSV_flightDistance2dVal[iJet] = -9999. ? -1. : JetInfo.TagVarCSV_flightDistance2dVal[iJet]);
      TagVarInfo.TagVarCSV_flightDistance2dSig     = (JetInfo.TagVarCSV_flightDistance2dSig[iJet] < -1000. ? -1. : JetInfo.TagVarCSV_flightDistance2dSig[iJet]);
      TagVarInfo.TagVarCSV_flightDistance3dVal     = (JetInfo.TagVarCSV_flightDistance3dVal[iJet] < -1000. ? -1. : JetInfo.TagVarCSV_flightDistance3dVal[iJet]);
      TagVarInfo.TagVarCSV_flightDistance3dSig     = (JetInfo.TagVarCSV_flightDistance3dSig[iJet] < -1000. ? -1. : JetInfo.TagVarCSV_flightDistance3dSig[iJet]);
      
      NORMALIZE(TagVarInfo.TagVarCSV_jetNTracks);
      NORMALIZE(TagVarInfo.TagVarCSV_jetNTracksEtaRel);
      NORMALIZE(TagVarInfo.TagVarCSV_trackSumJetEtRatio);
      NORMALIZE(TagVarInfo.TagVarCSV_trackSumJetDeltaR);
      NORMALIZE(TagVarInfo.TagVarCSV_trackSip2dValAboveCharm);
      NORMALIZE(TagVarInfo.TagVarCSV_trackSip2dSigAboveCharm);
      NORMALIZE(TagVarInfo.TagVarCSV_trackSip3dValAboveCharm);
      NORMALIZE(TagVarInfo.TagVarCSV_trackSip3dSigAboveCharm);
      NORMALIZE(TagVarInfo.TagVarCSV_vertexCategory);
      NORMALIZE(TagVarInfo.TagVarCSV_jetNSecondaryVertices);
      NORMALIZE(TagVarInfo.TagVarCSV_vertexMass);
      NORMALIZE(TagVarInfo.TagVarCSV_vertexNTracks);
      NORMALIZE(TagVarInfo.TagVarCSV_vertexEnergyRatio);
      NORMALIZE(TagVarInfo.TagVarCSV_vertexJetDeltaR);
      NORMALIZE(TagVarInfo.TagVarCSV_flightDistance2dVal);
      NORMALIZE(TagVarInfo.TagVarCSV_flightDistance2dSig);
      NORMALIZE(TagVarInfo.TagVarCSV_flightDistance3dVal);
      NORMALIZE(TagVarInfo.TagVarCSV_flightDistance3dSig);

      // loop over tracks to get IPs
      std::vector<float> IP2Ds;
      std::vector<float> IP3Ds;
      std::vector<float> PtRel;
      for (int iTrk = JetInfo.Jet_nFirstTrkTagVarCSV[iJet]; iTrk < JetInfo.Jet_nLastTrkTagVarCSV[iJet]; ++iTrk){
        IP2Ds.push_back( JetInfo.TagVarCSV_trackSip2dSig[iTrk] );
        IP3Ds.push_back( JetInfo.TagVarCSV_trackSip3dSig[iTrk] );
        PtRel.push_back( JetInfo.TagVarCSV_trackPtRel[iTrk] );
      }

      // loop over tracks to get trackEtaRel
      std::vector<float> etaRels; // stores |trackEtaRel|!
      for (int iTrk = JetInfo.Jet_nFirstTrkEtaRelTagVarCSV[iJet]; iTrk < JetInfo.Jet_nLastTrkEtaRelTagVarCSV[iJet]; ++iTrk)
        etaRels.push_back( fabs(JetInfo.TagVarCSV_trackEtaRel[iTrk]) );

      // sort the IP vectors in descending order and fill the branches based on the number of tracks
      std::sort( IP2Ds.begin(),IP2Ds.end(),std::greater<float>() );
      std::sort( IP3Ds.begin(),IP3Ds.end(),std::greater<float>() );
      std::sort( PtRel.begin(),PtRel.end(),std::greater<float>() );
      // sort etaRels in ascending order
      std::sort( etaRels.begin(),etaRels.end() ); //std::sort sorts in ascending order by default

      int numTracks = JetInfo.TagVarCSV_jetNTracks[iJet];
      int numEtaRelTracks = JetInfo.TagVarCSV_jetNTracksEtaRel[iJet];
      float dummyTrack = -99.;
      float dummyPtRel = -1.;
      float dummyEtaRel = -1.;
      // switch on the number of tracks in order to fill branches with a dummy if needed
      switch(numTracks){
        // if there are no selected tracks
        case 0:

          TagVarInfo.TagVarCSV_trackSip2dSig_0 = dummyTrack;
          TagVarInfo.TagVarCSV_trackSip2dSig_1 = dummyTrack;
          TagVarInfo.TagVarCSV_trackSip2dSig_2 = dummyTrack;
          TagVarInfo.TagVarCSV_trackSip2dSig_3 = dummyTrack;

          TagVarInfo.TagVarCSV_trackSip3dSig_0 = dummyTrack;
          TagVarInfo.TagVarCSV_trackSip3dSig_1 = dummyTrack;
          TagVarInfo.TagVarCSV_trackSip3dSig_2 = dummyTrack;
          TagVarInfo.TagVarCSV_trackSip3dSig_3 = dummyTrack;

          TagVarInfo.TagVarCSV_trackPtRel_0 = dummyPtRel;
          TagVarInfo.TagVarCSV_trackPtRel_1 = dummyPtRel;
          TagVarInfo.TagVarCSV_trackPtRel_2 = dummyPtRel;
          TagVarInfo.TagVarCSV_trackPtRel_3 = dummyPtRel;
          break;

        case 1:

          TagVarInfo.TagVarCSV_trackSip2dSig_0 = IP2Ds.at(0);
          TagVarInfo.TagVarCSV_trackSip2dSig_1 = dummyTrack;
          TagVarInfo.TagVarCSV_trackSip2dSig_2 = dummyTrack;
          TagVarInfo.TagVarCSV_trackSip2dSig_3 = dummyTrack;

          TagVarInfo.TagVarCSV_trackSip3dSig_0 = IP3Ds.at(0);
          TagVarInfo.TagVarCSV_trackSip3dSig_1 = dummyTrack;
          TagVarInfo.TagVarCSV_trackSip3dSig_2 = dummyTrack;
          TagVarInfo.TagVarCSV_trackSip3dSig_3 = dummyTrack;

          TagVarInfo.TagVarCSV_trackPtRel_0 = PtRel.at(0);
          TagVarInfo.TagVarCSV_trackPtRel_1 = dummyPtRel;
          TagVarInfo.TagVarCSV_trackPtRel_2 = dummyPtRel;
          TagVarInfo.TagVarCSV_trackPtRel_3 = dummyPtRel;
          break;

        case 2:

          TagVarInfo.TagVarCSV_trackSip2dSig_0 = IP2Ds.at(0);
          TagVarInfo.TagVarCSV_trackSip2dSig_1 = IP2Ds.at(1);
          TagVarInfo.TagVarCSV_trackSip2dSig_2 = dummyTrack;
          TagVarInfo.TagVarCSV_trackSip2dSig_3 = dummyTrack;

          TagVarInfo.TagVarCSV_trackSip3dSig_0 = IP3Ds.at(0);
          TagVarInfo.TagVarCSV_trackSip3dSig_1 = IP3Ds.at(1);
          TagVarInfo.TagVarCSV_trackSip3dSig_2 = dummyTrack;
          TagVarInfo.TagVarCSV_trackSip3dSig_3 = dummyTrack;

          TagVarInfo.TagVarCSV_trackPtRel_0 = PtRel.at(0);
          TagVarInfo.TagVarCSV_trackPtRel_1 = PtRel.at(1);
          TagVarInfo.TagVarCSV_trackPtRel_2 = dummyPtRel;
          TagVarInfo.TagVarCSV_trackPtRel_3 = dummyPtRel;
          break;

        case 3:

          TagVarInfo.TagVarCSV_trackSip2dSig_0 = IP2Ds.at(0);
          TagVarInfo.TagVarCSV_trackSip2dSig_1 = IP2Ds.at(1);
          TagVarInfo.TagVarCSV_trackSip2dSig_2 = IP2Ds.at(2);
          TagVarInfo.TagVarCSV_trackSip2dSig_3 = dummyTrack;

          TagVarInfo.TagVarCSV_trackSip3dSig_0 = IP3Ds.at(0);
          TagVarInfo.TagVarCSV_trackSip3dSig_1 = IP3Ds.at(1);
          TagVarInfo.TagVarCSV_trackSip3dSig_2 = IP3Ds.at(2);
          TagVarInfo.TagVarCSV_trackSip3dSig_3 = dummyTrack;

          TagVarInfo.TagVarCSV_trackPtRel_0 = PtRel.at(0);
          TagVarInfo.TagVarCSV_trackPtRel_1 = PtRel.at(1);
          TagVarInfo.TagVarCSV_trackPtRel_2 = PtRel.at(2);
          TagVarInfo.TagVarCSV_trackPtRel_3 = dummyPtRel;
          break;

        default:

          TagVarInfo.TagVarCSV_trackSip2dSig_0 = IP2Ds.at(0);
          TagVarInfo.TagVarCSV_trackSip2dSig_1 = IP2Ds.at(1);
          TagVarInfo.TagVarCSV_trackSip2dSig_2 = IP2Ds.at(2);
          TagVarInfo.TagVarCSV_trackSip2dSig_3 = IP2Ds.at(3);

          TagVarInfo.TagVarCSV_trackSip3dSig_0 = IP3Ds.at(0);
          TagVarInfo.TagVarCSV_trackSip3dSig_1 = IP3Ds.at(1);
          TagVarInfo.TagVarCSV_trackSip3dSig_2 = IP3Ds.at(2);
          TagVarInfo.TagVarCSV_trackSip3dSig_3 = IP3Ds.at(3);

          TagVarInfo.TagVarCSV_trackPtRel_0 = PtRel.at(0);
          TagVarInfo.TagVarCSV_trackPtRel_1 = PtRel.at(1);
          TagVarInfo.TagVarCSV_trackPtRel_2 = PtRel.at(2);
          TagVarInfo.TagVarCSV_trackPtRel_3 = PtRel.at(3);

      } // end switch on number of tracks for IP
          
      NORMALIZE(TagVarInfo.TagVarCSV_trackSip2dSig_0);
      NORMALIZE(TagVarInfo.TagVarCSV_trackSip2dSig_1);
      NORMALIZE(TagVarInfo.TagVarCSV_trackSip2dSig_2);
      NORMALIZE(TagVarInfo.TagVarCSV_trackSip2dSig_3);

      NORMALIZE(TagVarInfo.TagVarCSV_trackSip3dSig_0);
      NORMALIZE(TagVarInfo.TagVarCSV_trackSip3dSig_1);
      NORMALIZE(TagVarInfo.TagVarCSV_trackSip3dSig_2);
      NORMALIZE(TagVarInfo.TagVarCSV_trackSip3dSig_3);

      NORMALIZE(TagVarInfo.TagVarCSV_trackPtRel_0);
      NORMALIZE(TagVarInfo.TagVarCSV_trackPtRel_1);
      NORMALIZE(TagVarInfo.TagVarCSV_trackPtRel_2);
      NORMALIZE(TagVarInfo.TagVarCSV_trackPtRel_3);

      // switch on the number of etarel tracks in order to fill branches with a dummy if needed
      switch(numEtaRelTracks){
        // if there are no etarel tracks
        case 0:

          TagVarInfo.TagVarCSV_trackEtaRel_0 = dummyEtaRel;
          TagVarInfo.TagVarCSV_trackEtaRel_1 = dummyEtaRel;
          TagVarInfo.TagVarCSV_trackEtaRel_2 = dummyEtaRel;
          break;

        case 1:

          TagVarInfo.TagVarCSV_trackEtaRel_0 = etaRels.at(0);
          TagVarInfo.TagVarCSV_trackEtaRel_1 = dummyEtaRel;
          TagVarInfo.TagVarCSV_trackEtaRel_2 = dummyEtaRel;
          break;

        case 2:

          TagVarInfo.TagVarCSV_trackEtaRel_0 = etaRels.at(0);
          TagVarInfo.TagVarCSV_trackEtaRel_1 = etaRels.at(1);
          TagVarInfo.TagVarCSV_trackEtaRel_2 = dummyEtaRel;
          break;

        default:

          TagVarInfo.TagVarCSV_trackEtaRel_0 = etaRels.at(0);
          TagVarInfo.TagVarCSV_trackEtaRel_1 = etaRels.at(1);
          TagVarInfo.TagVarCSV_trackEtaRel_2 = etaRels.at(2);

      } //end switch on number of etarel tracks
      NORMALIZE(TagVarInfo.TagVarCSV_trackEtaRel_0);
      NORMALIZE(TagVarInfo.TagVarCSV_trackEtaRel_1);
      NORMALIZE(TagVarInfo.TagVarCSV_trackEtaRel_2);

      TagVarTree->Fill();
    }
    //----------------------------- End fat jet loop ----------------------------------------//

  }
  //----------------------------- End event loop ----------------------------------------//
}

// ------------ method called once each job just after ending the event loop  ------------
void 
TagVarExtractor::endJob() 
{
}

// ------------ method called when starting to processes a run  ------------
void 
TagVarExtractor::beginRun(edm::Run const&, edm::EventSetup const&)
{
}

// ------------ method called when ending the processing of a run  ------------
void 
TagVarExtractor::endRun(edm::Run const&, edm::EventSetup const&)
{
}

// ------------ method called when starting to processes a luminosity block  ------------
void 
TagVarExtractor::beginLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&)
{
}

// ------------ method called when ending the processing of a luminosity block  ------------
void 
TagVarExtractor::endLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&)
{
}

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void
TagVarExtractor::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  //The following says we do not know what parameters are allowed so do no validation
  // Please change this to state exactly what you do use, even if it is no parameters
  edm::ParameterSetDescription desc;
  desc.setUnknown();
  descriptions.addDefault(desc);
}

//define this as a plug-in
DEFINE_FWK_MODULE(TagVarExtractor);
