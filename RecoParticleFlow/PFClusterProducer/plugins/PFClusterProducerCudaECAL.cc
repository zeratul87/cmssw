#include "PFClusterProducerCudaECAL.h"
#include "RecoParticleFlow/PFClusterProducer/plugins/PFClusterCudaECAL.h"
#include <TFile.h>
#include <TH1F.h>
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "HeterogeneousCore/CUDAUtilities/interface/device_unique_ptr.h"
#include "HeterogeneousCore/CUDAUtilities/interface/host_unique_ptr.h"


#ifdef PFLOW_DEBUG
#define LOGVERB(x) edm::LogVerbatim(x)
#define LOGWARN(x) edm::LogWarning(x)
#define LOGERR(x) edm::LogError(x)
#define LOGDRESSED(x) edm::LogInfo(x)
#else
#define LOGVERB(x) LogTrace(x)
#define LOGWARN(x) edm::LogWarning(x)
#define LOGERR(x) edm::LogError(x)
#define LOGDRESSED(x) LogDebug(x)
#endif

PFClusterProducerCudaECAL::PFClusterProducerCudaECAL(const edm::ParameterSet& conf)
  : 

  _prodInitClusters(conf.getUntrackedParameter<bool>("prodInitialClusters", false)) {
  _rechitsLabel = consumes<reco::PFRecHitCollection>(conf.getParameter<edm::InputTag>("recHitsSource"));


  //setup rechit cleaners
  const edm::VParameterSet& cleanerConfs = conf.getParameterSetVector("recHitCleaners");

  for (const auto& conf : cleanerConfs) {
    const std::string& cleanerName = conf.getParameter<std::string>("algoName");
    _cleaners.emplace_back(RecHitTopologicalCleanerFactory::get()->create(cleanerName, conf));
  }

  edm::ConsumesCollector sumes = consumesCollector();

  // setup seed finding
  const edm::ParameterSet& sfConf = conf.getParameterSet("seedFinder");
  const std::string& sfName = sfConf.getParameter<std::string>("algoName");
  _seedFinder = SeedFinderFactory::get()->create(sfName, sfConf);

  const edm::VParameterSet& seedFinderConfs = sfConf.getParameterSetVector("thresholdsByDetector"); 
  

  if (conf.exists("allCellsPositionCalc")) {
    const edm::ParameterSet& acConf = conf.getParameterSet("allCellsPositionCalc");
    const std::string& algoac = acConf.getParameter<std::string>("algoName");
    _allCellsPosCalc = PFCPositionCalculatorFactory::get()->create(algoac, acConf);
  }
 
  //setup topo cluster builder
  const edm::ParameterSet& initConf = conf.getParameterSet("initialClusteringStep");
  const std::string& initName = initConf.getParameter<std::string>("algoName");
  _initialClustering = InitialClusteringStepFactory::get()->create(initName, initConf, sumes);
  //setup pf cluster builder if requested
  const edm::ParameterSet& pfcConf = conf.getParameterSet("pfClusterBuilder");
  if (!pfcConf.empty()) {
    const std::string& pfcName = pfcConf.getParameter<std::string>("algoName");
    _pfClusterBuilder = PFClusterBuilderFactory::get()->create(pfcName, pfcConf);
  }
  //setup (possible) recalcuation of positions
  const edm::ParameterSet& pConf = conf.getParameterSet("positionReCalc");
  if (!pConf.empty()) {
    const std::string& pName = pConf.getParameter<std::string>("algoName");
    _positionReCalc = PFCPositionCalculatorFactory::get()->create(pName, pConf);
  }
  // see if new need to apply corrections, setup if there.
  const edm::ParameterSet& cConf = conf.getParameterSet("energyCorrector");
  if (!cConf.empty()) {
    const std::string& cName = cConf.getParameter<std::string>("algoName");
    _energyCorrector = PFClusterEnergyCorrectorFactory::get()->create(cName, cConf);
  }

  if (_prodInitClusters) {
    produces<reco::PFClusterCollection>("initialClusters");
  }
  produces<reco::PFClusterCollection>();
 
  
}

PFClusterProducerCudaECAL::~PFClusterProducerCudaECAL()
{
  
}

void PFClusterProducerCudaECAL::beginLuminosityBlock(const edm::LuminosityBlock& lumi, const edm::EventSetup& es) {
  _initialClustering->update(es);
  if (_pfClusterBuilder)
    _pfClusterBuilder->update(es);
  if (_positionReCalc)
    _positionReCalc->update(es);
}

void PFClusterProducerCudaECAL::produce(edm::Event& e, const edm::EventSetup& es) {
  _initialClustering->reset();
  if (_pfClusterBuilder)
    _pfClusterBuilder->reset();

  edm::Handle<reco::PFRecHitCollection> rechits;
  e.getByToken(_rechitsLabel, rechits);
  
  _initialClustering->updateEvent(e);

  std::vector<bool> mask(rechits->size(), true);
  for (const auto& cleaner : _cleaners) {
    cleaner->clean(rechits, mask);
  }

 size_t rh_size = rechits->size();
  int numbytes_float = rh_size*sizeof(float);
  int numbytes_int = rh_size*sizeof(int);

  std::vector<float>                                    h_cuda_pfRhFrac=std::vector<float>(rh_size*50,-1.);
  std::vector<float>                                    h_cuda_pcRhFrac=std::vector<float>(rh_size*50,-1.);
  std::vector<int>                                      h_cuda_pfRhFracInd=std::vector<int>(rh_size*50,-1);
  std::vector<int>                                      h_cuda_pfNeighEightInd=std::vector<int>(rh_size*8,0);
  std::vector<int>                                      h_cuda_pcRhFracInd=std::vector<int>(rh_size*50,-1);

  std::vector<float>                                    h_cuda_pfrh_x=std::vector<float>(rh_size,0);
  std::vector<float>                                    h_cuda_pfrh_y=std::vector<float>(rh_size,0);
  std::vector<float>                                    h_cuda_pfrh_z=std::vector<float>(rh_size,0);
  std::vector<float>                                    h_cuda_pfrh_energy=std::vector<float>(rh_size,0);
  std::vector<float>                                    h_cuda_pfrh_pt2=std::vector<float>(rh_size,0);
  std::vector<int>                                      h_cuda_pfrh_topoId=std::vector<int>(rh_size,-1);
  std::vector<int>                                      h_cuda_pfrh_isSeed=std::vector<int>(rh_size,0);
  std::vector<int>                                      h_cuda_pfrh_layer=std::vector<int>(rh_size,-999);

auto d_cuda_pfrh_x = cms::cuda::make_device_unique<float[]>(numbytes_float, nullptr);
auto d_cuda_pfrh_y = cms::cuda::make_device_unique<float[]>(numbytes_float, nullptr);
auto d_cuda_pfrh_z = cms::cuda::make_device_unique<float[]>(numbytes_float, nullptr);
auto d_cuda_pfrh_energy = cms::cuda::make_device_unique<float[]>(numbytes_float, nullptr);
auto d_cuda_pfrh_pt2 = cms::cuda::make_device_unique<float[]>(numbytes_float, nullptr);
auto d_cuda_pfRhFrac = cms::cuda::make_device_unique<float[]>(numbytes_float*50, nullptr);
auto d_cuda_pcRhFrac = cms::cuda::make_device_unique<float[]>(numbytes_float*50, nullptr);

auto d_cuda_pfrh_topoId = cms::cuda::make_device_unique<int[]>(numbytes_int, nullptr);
auto d_cuda_pfrh_isSeed = cms::cuda::make_device_unique<int[]>(numbytes_int, nullptr);
auto d_cuda_pfrh_layer = cms::cuda::make_device_unique<int[]>(numbytes_int, nullptr);
auto d_cuda_pfNeighEightInd = cms::cuda::make_device_unique<int[]>(numbytes_int*8, nullptr);

auto d_cuda_pfRhFracInd = cms::cuda::make_device_unique<int[]>(numbytes_int*50, nullptr);
auto d_cuda_pcRhFracInd = cms::cuda::make_device_unique<int[]>(numbytes_int*50, nullptr);

  int p=0; 
  for (auto rh: *rechits){

    h_cuda_pfrh_x[p]=rh.position().x();
    h_cuda_pfrh_y[p]=rh.position().y();
    h_cuda_pfrh_z[p]=rh.position().z();
    h_cuda_pfrh_energy[p]=rh.energy();
    h_cuda_pfrh_pt2[p]=rh.pt2();
    h_cuda_pfrh_layer[p]=(int)rh.layer();
    h_cuda_pfrh_topoId[p]=p;

    auto theneighboursEight = rh.neighbours8();
    int z = 0;
    for(auto nh: theneighboursEight)
      {
	h_cuda_pfNeighEightInd[8*p+z] = nh;
	z++;
      }
    for(int l=z; l<8; l++)
      {
	h_cuda_pfNeighEightInd[8*p+l] = -1;
      }

    p++;
  }//end of rechit loop  
  //std::cout<<"p: "<<p<<std::endl;
  
  
  cudaCheck(cudaMemcpy(d_cuda_pfrh_x.get(), h_cuda_pfrh_x.data(), numbytes_float, cudaMemcpyHostToDevice));
  cudaCheck(cudaMemcpy(d_cuda_pfrh_y.get(), h_cuda_pfrh_y.data(), numbytes_float, cudaMemcpyHostToDevice));
  cudaCheck(cudaMemcpy(d_cuda_pfrh_z.get(), h_cuda_pfrh_z.data(), numbytes_float, cudaMemcpyHostToDevice));
  cudaCheck(cudaMemcpy(d_cuda_pfrh_energy.get(), h_cuda_pfrh_energy.data(), numbytes_float, cudaMemcpyHostToDevice));
  cudaCheck(cudaMemcpy(d_cuda_pfrh_pt2.get(), h_cuda_pfrh_pt2.data(), numbytes_float, cudaMemcpyHostToDevice));
  cudaCheck(cudaMemcpy(d_cuda_pfrh_topoId.get(), h_cuda_pfrh_topoId.data(), numbytes_int, cudaMemcpyHostToDevice));
  cudaCheck(cudaMemcpy(d_cuda_pfrh_isSeed.get(), h_cuda_pfrh_isSeed.data(), numbytes_int, cudaMemcpyHostToDevice));
  cudaCheck(cudaMemcpy(d_cuda_pfrh_layer.get(), h_cuda_pfrh_layer.data(), numbytes_int, cudaMemcpyHostToDevice));
  cudaCheck(cudaMemcpy(d_cuda_pfNeighEightInd.get(), h_cuda_pfNeighEightInd.data(), numbytes_int*8, cudaMemcpyHostToDevice));  

  
  cudaCheck(cudaMemcpy(d_cuda_pfRhFrac.get(), h_cuda_pfRhFrac.data(), numbytes_float*50, cudaMemcpyHostToDevice));
  cudaCheck(cudaMemcpy(d_cuda_pcRhFrac.get(), h_cuda_pcRhFrac.data(), numbytes_float*50, cudaMemcpyHostToDevice));
  cudaCheck(cudaMemcpy(d_cuda_pfRhFracInd.get(), h_cuda_pfRhFracInd.data(), numbytes_int*50, cudaMemcpyHostToDevice));
  cudaCheck(cudaMemcpy(d_cuda_pcRhFracInd.get(), h_cuda_pcRhFracInd.data(), numbytes_int*50, cudaMemcpyHostToDevice));

  PFClusterCudaECAL::PFRechitToPFCluster_ECAL(rh_size, 
					      d_cuda_pfrh_x.get(),  
					      d_cuda_pfrh_y.get(),  
					      d_cuda_pfrh_z.get(), 
					      d_cuda_pfrh_energy.get(), 
					      d_cuda_pfrh_pt2.get(), 	
					      d_cuda_pfrh_isSeed.get(),
					      d_cuda_pfrh_topoId.get(),
					      d_cuda_pfrh_layer.get(), 
					      d_cuda_pfNeighEightInd.get(), 
					      d_cuda_pfRhFrac.get(), 
					      d_cuda_pfRhFracInd.get(), 
					      d_cuda_pcRhFracInd.get(),
					      d_cuda_pcRhFrac.get()
					      );
     
    
  cudaMemcpy(h_cuda_pcRhFracInd.data()    , d_cuda_pcRhFracInd.get()  , numbytes_int*50 , cudaMemcpyDeviceToHost);  
  cudaMemcpy(h_cuda_pcRhFrac.data()       , d_cuda_pcRhFrac.get()  , numbytes_float*50 , cudaMemcpyDeviceToHost);  
  cudaMemcpy(h_cuda_pfrh_isSeed.data()    , d_cuda_pfrh_isSeed.get()  , numbytes_int , cudaMemcpyDeviceToHost);  
  
  

  auto pfClustersFromCuda = std::make_unique<reco::PFClusterCollection>();
  pfClustersFromCuda.reset(new reco::PFClusterCollection);
  for(int n=0; n<(int)rh_size; n++){
    if(h_cuda_pfrh_isSeed[n]==1){
      reco::PFCluster temp;
      temp.setSeed((*rechits)[n].detId());
      for(int k=0;k<50;k++){
	if(h_cuda_pcRhFracInd[n*50+k] > -1){
	  const reco::PFRecHitRef& refhit = reco::PFRecHitRef(rechits,h_cuda_pcRhFracInd[n*50+k]);
	  temp.addRecHitFraction( reco::PFRecHitFraction(refhit, h_cuda_pcRhFrac[n*50+k]) );
	}
	if(h_cuda_pcRhFracInd[n*50+k] < 0.) break;
      }    
      pfClustersFromCuda->push_back(temp);
    }   
  }
  _positionReCalc->calculateAndSetPositions(*pfClustersFromCuda);
  
  if (_energyCorrector) {
    _energyCorrector->correctEnergies(*pfClustersFromCuda);
  }

  if (_prodInitClusters)
    e.put(std::move(pfClustersFromCuda), "initialClusters");
  e.put(std::move(pfClustersFromCuda));
}



