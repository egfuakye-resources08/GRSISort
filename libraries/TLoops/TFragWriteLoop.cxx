#include "TFragWriteLoop.h"

#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>

#include "TFile.h"
#include "TThread.h"

#include "GValue.h"
#include "TChannel.h"
#include "TGRSIRunInfo.h"
#include "TThread.h"
#include "TTreeFillMutex.h"

TFragWriteLoop* TFragWriteLoop::Get(std::string name, std::string fOutputFilename){
  if(name.length()==0){
    name = "write_loop";
  }

  StoppableThread* thread = StoppableThread::Get(name);
  if(!thread){
    if(fOutputFilename.length()==0){
      fOutputFilename = "temp.root";
    }
    thread = new TFragWriteLoop(name,fOutputFilename);
  }

  return dynamic_cast<TFragWriteLoop*>(thread);
}

TFragWriteLoop::TFragWriteLoop(std::string name, std::string fOutputFilename)
  : StoppableThread(name),
    fOutputFile(NULL), fEventTree(NULL), fBadEventTree(NULL), fScalerTree(NULL),
	 fEventAddress(NULL), fBadEventAddress(NULL), fScalerAddress(NULL),
    fItemsHandled(0),fInputQueueSize(0),
    fInputQueue(std::make_shared<ThreadsafeQueue<std::shared_ptr<TFragment> > >()),
    fBadInputQueue(std::make_shared<ThreadsafeQueue<std::shared_ptr<TFragment> > >()),
    fScalerInputQueue(std::make_shared<ThreadsafeQueue<std::shared_ptr<TEpicsFrag> > >()) {

  if(fOutputFilename != "/dev/null"){
    TThread::Lock();

    //TPreserveGDirectory preserve;
    fOutputFile = new TFile(fOutputFilename.c_str(),"RECREATE");

    fEventTree = new TTree("FragmentTree","FragmentTree");
    fEventTree->Branch("TFragment", &fEventAddress);

    fBadEventTree = new TTree("BadFragmentTree","BadFragmentTree");
    fBadEventTree->Branch("TFragment", &fBadEventAddress);

    fScalerTree = new TTree("EpicsTree","EpicsTree");
    fScalerTree->Branch("TEpicsFrag", &fScalerAddress);

    TThread::UnLock();
  }

}

TFragWriteLoop::~TFragWriteLoop() {
  Write();
}

void TFragWriteLoop::ClearQueue() {
  while(fInputQueue->Size()){
    std::shared_ptr<TFragment> event;
    fInputQueue->Pop(event);
  }
}

std::string TFragWriteLoop::Status() {
	std::stringstream ss;
	ss<<Name()<<":\t"<<std::setw(8)<<GetItemsPushed()<<"/"<<(fInputQueueSize>0 ? fInputQueueSize+GetItemsPushed():GetItemsPushed());
	return ss.str();
}

std::string TFragWriteLoop::EndStatus() {
	std::stringstream ss;
	ss<<"\r"<<Name()<<":\t"<<std::setw(8)<<GetItemsPushed()<<"/"<<(fInputQueueSize>0 ? fInputQueueSize+GetItemsPushed():GetItemsPushed())<<std::endl;;
	return ss.str();
}

bool TFragWriteLoop::Iteration() {
	std::shared_ptr<TFragment> event;
	fInputQueueSize = fInputQueue->Pop(event,0);

	std::shared_ptr<TFragment> badEvent;
	fBadInputQueue->Pop(badEvent,0);

	std::shared_ptr<TEpicsFrag> scaler;
	fScalerInputQueue->Pop(scaler,0);

	bool hasAnything = event || badEvent || scaler;
	bool allParentsDead = (fInputQueue->IsFinished() && fBadInputQueue->IsFinished() &&
			fScalerInputQueue->IsFinished());

	if(event) {
		WriteEvent(event);
		fItemsHandled++;
	}

	if(badEvent) {
		WriteBadEvent(badEvent);
	}

	if(scaler) {
		WriteScaler(scaler);
	}

	if(hasAnything) {
		return true;
	} else if(allParentsDead) {
		return false;
	} else {
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		return true;
	}
}

void TFragWriteLoop::Write() {
	if(fOutputFile){
		fOutputFile->cd();
		fEventTree->Write(fEventTree->GetName(), TObject::kOverwrite);
		fBadEventTree->Write(fBadEventTree->GetName(), TObject::kOverwrite);
		fScalerTree->Write(fScalerTree->GetName(), TObject::kOverwrite);
		if(GValue::Size()) {
			GValue::Get()->Write();
		}

		if(TChannel::GetNumberOfChannels()) {
			//TChannel::GetDefaultChannel()->Write();
			TChannel::WriteToRoot();
		}

		TGRSIRunInfo::Get()->WriteToRoot(fOutputFile);
		TPPG::Get()->Write();

		fOutputFile->Close();
		fOutputFile->Delete();
	}
}

void TFragWriteLoop::WriteEvent(std::shared_ptr<TFragment> event) {
	if(fEventTree) {
		fEventAddress = event.get();
		std::lock_guard<std::mutex> lock(ttree_fill_mutex);
		fEventTree->Fill();
		fEventAddress = NULL;
	} else {
		std::cout<<__PRETTY_FUNCTION__<<": no fragment tree!"<<std::endl;
	}
}

void TFragWriteLoop::WriteBadEvent(std::shared_ptr<TFragment> event) {
	if(fBadEventTree) {
		fBadEventAddress = event.get();
		std::lock_guard<std::mutex> lock(ttree_fill_mutex);
		fBadEventTree->Fill();
		fBadEventAddress = NULL;
	}
}

void TFragWriteLoop::WriteScaler(std::shared_ptr<TEpicsFrag> scaler) {
	if(fScalerTree) {
		fScalerAddress = scaler.get();
		std::lock_guard<std::mutex> lock(ttree_fill_mutex);
		fScalerTree->Fill();
		fScalerAddress = NULL;
	}
}

