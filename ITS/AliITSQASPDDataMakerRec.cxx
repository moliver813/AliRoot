/**************************************************************************
 * Copyright(c) 2007-2009, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/
/* $Id$  */
//  *************************************************************
//  Checks the quality assurance 
//  by comparing with reference data
//  contained in a DB
//  -------------------------------------------------------------
//  W. Ferrarese + P. Cerello Feb 2008
//  INFN Torino
//  M. Nicassio D. Elia INFN Bari March 2008
//  maria.nicassio@ba.infn.it
    

// --- ROOT system ---
#include <TTree.h>
#include <TH1.h>
#include <TH2.h>
#include <TMath.h>
// --- Standard library ---

// --- AliRoot header files ---
#include "AliITSQADataMakerRec.h"
#include "AliITSQASPDDataMakerRec.h"
#include "AliLog.h"
#include "AliQAv1.h"
#include "AliRawReader.h"
#include "AliITSRawStreamSPD.h"
#include "AliITSRawStreamSPDErrorLog.h"
#include "AliITSdigitSPD.h"
#include "AliITSRecPoint.h"

ClassImp(AliITSQASPDDataMakerRec)

//____________________________________________________________________________
AliITSQASPDDataMakerRec::AliITSQASPDDataMakerRec(AliITSQADataMakerRec *aliITSQADataMakerRec, Bool_t kMode, Short_t ldc, AliITSRawStreamSPDErrorLog *aliITSRawStreamSPDErrorLog) :
TObject(),
fAliITSQADataMakerRec(aliITSQADataMakerRec),
fkOnline(kMode),
fLDC(ldc),
fSPDhRawsTask(0),
fSPDhDigitsTask(0),
fSPDhRecPointsTask(0),
fGenRawsOffset(0),
fGenDigitsOffset(0),
fGenRecPointsOffset(0),
fAdvLogger(aliITSRawStreamSPDErrorLog) 
{
  //ctor used to discriminate OnLine-Offline analysis  
}

//____________________________________________________________________________ 
AliITSQASPDDataMakerRec::AliITSQASPDDataMakerRec(const AliITSQASPDDataMakerRec& qadm) :
TObject(),
fAliITSQADataMakerRec(qadm.fAliITSQADataMakerRec),
fkOnline(qadm.fkOnline),
fLDC(qadm.fLDC),
fSPDhRawsTask(qadm.fSPDhRawsTask),
fSPDhDigitsTask(qadm.fSPDhDigitsTask),
fSPDhRecPointsTask(qadm.fSPDhRecPointsTask),
fGenRawsOffset(qadm.fGenRawsOffset),
fGenDigitsOffset(qadm.fGenDigitsOffset),
fGenRecPointsOffset(qadm.fGenRecPointsOffset),
fAdvLogger(qadm.fAdvLogger)
{
  //copy ctor 
  fAliITSQADataMakerRec->SetName((const char*)qadm.fAliITSQADataMakerRec->GetName()) ; 
  fAliITSQADataMakerRec->SetTitle((const char*)qadm.fAliITSQADataMakerRec->GetTitle());
  }

//__________________________________________________________________
AliITSQASPDDataMakerRec::~AliITSQASPDDataMakerRec(){
  // destructor
//  delete fAdvLogger;
}
//__________________________________________________________________

AliITSQASPDDataMakerRec& AliITSQASPDDataMakerRec::operator = (const AliITSQASPDDataMakerRec& qac )
{
  // Equal operator.
  this->~AliITSQASPDDataMakerRec();
  new(this) AliITSQASPDDataMakerRec(qac);
  return *this;
}

//____________________________________________________________________________ 
void AliITSQASPDDataMakerRec::StartOfDetectorCycle()
{
  //Detector specific actions at start of cycle
  AliDebug(AliQAv1::GetQADebugLevel(),"AliITSQADM::Start of SPD Cycle\n");
}

//____________________________________________________________________________ 
void AliITSQASPDDataMakerRec::EndOfDetectorCycle(AliQAv1::TASKINDEX_t /*task*/, TObjArray* /*list*/)
{
  // launch the QA checking
  AliDebug(AliQAv1::GetQADebugLevel(),"AliITSDM instantiates checker with Run(AliQAv1::kITS, task, list)\n"); 
  
  //AliQAChecker::Instance()->Run( AliQAv1::kITS , task, list);
}

//____________________________________________________________________________ 
Int_t AliITSQASPDDataMakerRec::InitRaws()
{ 
  // Initialization for RAW data - SPD -
  const Bool_t expert   = kTRUE ; 
  const Bool_t saveCorr = kTRUE ; 
  const Bool_t image    = kTRUE ; 
  Int_t rv = 0 ; 
//  fGenRawsOffset = (fAliITSQADataMakerRec->fRawsQAList[AliRecoParam::kDefault])->GetEntries();
  fAdvLogger = new AliITSRawStreamSPDErrorLog();  
  AliDebug(AliQAv1::GetQADebugLevel(), "Book Offline Histograms for SPD\n ");

  Char_t name[50];
  Char_t title[50];

  TH1F *hlayer = new TH1F("SPDLayPattern_SPD","Layer map - SPD",6,0.,6.);
  hlayer->GetXaxis()->SetTitle("Layer number");
  hlayer->GetYaxis()->SetTitle("Entries");
  rv = fAliITSQADataMakerRec->Add2RawsList(hlayer, 0+fGenRawsOffset, expert, !image, !saveCorr);
  fSPDhRawsTask++;

  TH1F **hmod = new TH1F*[2];
  TH2F **hhitMap = new TH2F*[20];
  TH1F **herrors = new TH1F*[20];
  for (Int_t iLay=0; iLay<2; iLay++) {
    sprintf(name,"SPDModPattern_SPD%d",iLay+1);
    sprintf(title,"Module map - SPD Layer %d",iLay+1);
    hmod[iLay]=new TH1F(name,title,fgknSPDmodules,0,fgknSPDmodules);
    hmod[iLay]->GetXaxis()->SetTitle("Module number");
    hmod[iLay]->GetYaxis()->SetTitle("Entries");
    rv = fAliITSQADataMakerRec->Add2RawsList(hmod[iLay], 1+iLay+fGenRawsOffset, !expert, image, !saveCorr);
    fSPDhRawsTask++;
  }
  for (Int_t iDDL=0; iDDL<20; iDDL++) {
    sprintf(name,"SPDHitMap_SPD_DDL%d",iDDL+1);
    sprintf(title,"Hit map - SPD DDL %d",iDDL+1);
    hhitMap[iDDL]=new TH2F(name,title,320,0,10*32,1536,0,6*256);
    hhitMap[iDDL]->GetXaxis()->SetTitle("Column");
    hhitMap[iDDL]->GetYaxis()->SetTitle("Row");
    rv = fAliITSQADataMakerRec->Add2RawsList(hhitMap[iDDL], 3+(2*iDDL)+fGenRawsOffset, expert, !image, !saveCorr);
    fSPDhRawsTask++;
    sprintf(name,"SPDErrors_SPD_DDL%d",iDDL+1);
    sprintf(title,"Error codes - SPD DDL %d",iDDL+1);
    herrors[iDDL] = new TH1F (name,title,fAdvLogger->GetNrErrorCodes(),0,fAdvLogger->GetNrErrorCodes());
    herrors[iDDL]->SetXTitle("Error Code");
    herrors[iDDL]->SetYTitle("Nr of errors");
    rv = fAliITSQADataMakerRec->Add2RawsList(herrors[iDDL], 4+(2*iDDL)+fGenRawsOffset, expert, !image, !saveCorr);
    fSPDhRawsTask++;
  }

  TH1F** hMultSPDhits = new TH1F*[2];
  for (Int_t iLay=0; iLay<2; iLay++) {
    sprintf(name,"SPDHitsMultiplicity_SPD%d",iLay+1);
    sprintf(title,"Hit multiplicity - SPD Layer %d",iLay+1);
    hMultSPDhits[iLay]=new TH1F(name,title,200,0.,200.);
    hMultSPDhits[iLay]->GetXaxis()->SetTitle("Hit multiplicity");
    hMultSPDhits[iLay]->GetYaxis()->SetTitle("Entries");
    rv = fAliITSQADataMakerRec->Add2RawsList(hMultSPDhits[iLay], 43+iLay+fGenRawsOffset, expert, !image, !saveCorr);
    fSPDhRawsTask++;
  }

  TH2F *hMultSPDhits2MultSPDhits1 
         = new TH2F("SPDHitMultCorrelation_SPD","Hit multiplicity correlation - SPD",200,0.,200.,200,0.,200.);
  hMultSPDhits2MultSPDhits1->GetXaxis()->SetTitle("Hit multiplicity (Layer 1)");
  hMultSPDhits2MultSPDhits1->GetYaxis()->SetTitle("Hit multiplicity (Layer 2)");
  rv = fAliITSQADataMakerRec->Add2RawsList(hMultSPDhits2MultSPDhits1, 45+fGenRawsOffset, !expert, image, !saveCorr);
  fSPDhRawsTask++;
 
  AliDebug(AliQAv1::GetQADebugLevel(),Form("%d SPD Raws histograms booked\n",fSPDhRawsTask));
  return rv ; 
}

//____________________________________________________________________________
Int_t AliITSQASPDDataMakerRec::MakeRaws(AliRawReader* rawReader)
{ 
  // Fill QA for RAW - SPD -
  Int_t rv = 0 ; 
  // Check id histograms already created for this Event Specie
  if ( ! fAliITSQADataMakerRec->GetRawsData(fGenRawsOffset) )
    rv = InitRaws() ;
  
  rawReader->Reset();
  AliITSRawStreamSPD *rawStreamSPD = new AliITSRawStreamSPD(rawReader);
  rawStreamSPD->ActivateAdvancedErrorLog(kTRUE,fAdvLogger);

  Int_t nDigitsL1 = 0;
  Int_t nDigitsL2 = 0;
  Int_t iEq;
  Int_t iLayer;
  Int_t iHalfStave, iChip;
  Int_t col, row; 
  UInt_t module, colM, rowM;
  while(rawStreamSPD->Next()) {

    iEq = rawReader->GetDDLID();
    if (iEq>=0 && iEq<20) {
      iHalfStave = rawStreamSPD->GetHalfStaveNr();
      iChip = rawStreamSPD->GetChipAddr();
      col  = rawStreamSPD->GetChipCol();
      row  = rawStreamSPD->GetChipRow();

      rawStreamSPD->OnlineToOffline(iEq, iHalfStave, iChip, col, row, module, colM, rowM);

      if (iHalfStave>=0 && iHalfStave<2) iLayer=0;
      else iLayer=1;
      
      fAliITSQADataMakerRec->GetRawsData(0+fGenRawsOffset)->Fill(iLayer);
      if (iLayer==0) {
        fAliITSQADataMakerRec->GetRawsData(1+fGenRawsOffset)->Fill(module);
        nDigitsL1++;
      } else {
        fAliITSQADataMakerRec->GetRawsData(2+fGenRawsOffset)->Fill(module);
        nDigitsL2++;
      }
      
      fAliITSQADataMakerRec->GetRawsData((2*iEq)+3+fGenRawsOffset)->Fill(colM+(module%2)*160,rowM+iHalfStave*256);
    }

  }
  for (Int_t ieq=0; ieq<20; ieq++)
    for (UInt_t ierr=0; ierr<fAdvLogger->GetNrErrorCodes(); ierr++)
      fAliITSQADataMakerRec->GetRawsData((2*ieq)+4+fGenRawsOffset)->Fill(ierr,fAdvLogger->GetNrErrors(ierr,ieq));

  fAdvLogger->Reset();
 
  fAliITSQADataMakerRec->GetRawsData(43+fGenRawsOffset)->Fill(nDigitsL1);
  fAliITSQADataMakerRec->GetRawsData(44+fGenRawsOffset)->Fill(nDigitsL2);
  fAliITSQADataMakerRec->GetRawsData(45+fGenRawsOffset)->Fill(nDigitsL1,nDigitsL2);
  
  delete rawStreamSPD;  
  AliDebug(AliQAv1::GetQADebugLevel(),Form("Event completed, %d raw digits read",nDigitsL1+nDigitsL2));
  return rv ; 
}

//____________________________________________________________________________ 
Int_t AliITSQASPDDataMakerRec::InitDigits()
{ 
  // Initialization for DIGIT data - SPD -
  const Bool_t expert   = kTRUE ; 
  const Bool_t image    = kTRUE ;
  Int_t rv = 0 ; 
//  fGenDigitsOffset = (fAliITSQADataMakerRec->fDigitsQAList[AliRecoParam::kDefault])->GetEntries();
  //fSPDhDigitsTask must be incremented by one unit every time a histogram is ADDED to the QA List
  
  Char_t name[50];
  Char_t title[50];
  
  TH1F *hlayer = new TH1F("SPDLayPattern_SPD","Layer map - SPD",6,0.,6.);
  hlayer->GetXaxis()->SetTitle("Layer number");
  hlayer->GetYaxis()->SetTitle("Entries");
  rv = fAliITSQADataMakerRec->Add2DigitsList(hlayer,fGenDigitsOffset, expert, !image);
  fSPDhDigitsTask++;
  
  TH1F **hmod = new TH1F*[2];
  for (Int_t iLay=0; iLay<2; iLay++) {
    sprintf(name,"SPDModPattern_SPD%d",iLay+1);
    sprintf(title,"Module map - SPD Layer %d",iLay+1);
    hmod[iLay]=new TH1F(name,title,240,0,240);
    hmod[iLay]->GetXaxis()->SetTitle("Module number");
    hmod[iLay]->GetYaxis()->SetTitle("Entries");
    rv = fAliITSQADataMakerRec->Add2DigitsList(hmod[iLay],1+iLay+fGenDigitsOffset, !expert, image);
    fSPDhDigitsTask++;
  }
  
  TH1F *hcolumns = new TH1F("SPDColumns_SPD","Columns - SPD",160,0.,160.);
  hcolumns->GetXaxis()->SetTitle("Column number");
  hcolumns->GetYaxis()->SetTitle("Entries");
  rv = fAliITSQADataMakerRec->Add2DigitsList(hcolumns,3+fGenDigitsOffset, expert, !image);
  fSPDhDigitsTask++;
  
  TH1F *hrows = new TH1F("SPDRows_SPD","Rows - SPD",256,0.,256.);
  hrows->GetXaxis()->SetTitle("Row number");
  hrows->GetYaxis()->SetTitle("Entries");
  rv = fAliITSQADataMakerRec->Add2DigitsList(hrows,4+fGenDigitsOffset, expert, !image);
  fSPDhDigitsTask++;
  
  TH1F** hMultSPDdigits = new TH1F*[2];
  for (Int_t iLay=0; iLay<2; ++iLay) {
    sprintf(name,"SPDDigitMultiplicity_SPD%d",iLay+1);
    sprintf(title,"Digit multiplicity - SPD Layer %d",iLay+1);
    hMultSPDdigits[iLay]=new TH1F(name,title,200,0.,200.);
    hMultSPDdigits[iLay]->GetXaxis()->SetTitle("Digit multiplicity");
    hMultSPDdigits[iLay]->GetYaxis()->SetTitle("Entries");
    rv = fAliITSQADataMakerRec->Add2DigitsList(hMultSPDdigits[iLay], 5+iLay+fGenDigitsOffset, !expert, image);
    fSPDhDigitsTask++;
  }
  
  TH2F *hMultSPDdig2MultSPDdig1 
    = new TH2F("SPDDigitMultCorrelation_SPD","Digit multiplicity correlation - SPD",200,0.,200.,200,0.,200.);
  hMultSPDdig2MultSPDdig1->GetXaxis()->SetTitle("Digit multiplicity (Layer 1)");
  hMultSPDdig2MultSPDdig1->GetYaxis()->SetTitle("Digit multiplicity (Layer 2)");
  rv = fAliITSQADataMakerRec->Add2DigitsList(hMultSPDdig2MultSPDdig1,7+fGenDigitsOffset, !expert, image);
  fSPDhDigitsTask++;
  
  AliDebug(AliQAv1::GetQADebugLevel(),Form("%d SPD Digits histograms booked\n",fSPDhDigitsTask));
  return rv ; 
}

//____________________________________________________________________________
Int_t AliITSQASPDDataMakerRec::MakeDigits(TTree *digits)
{ 
  // Fill QA for DIGIT - SPD -
  Int_t rv = 0 ; 
  // Check id histograms already created for this Event Specie
  if ( ! fAliITSQADataMakerRec->GetDigitsData(fGenDigitsOffset) )
    rv = InitDigits() ;

//  AliITS *fITS  = (AliITS*)gAlice->GetModule("ITS");
//  fITS->SetTreeAddress();
//  TClonesArray *iITSdigits  = fITS->DigitsAddress(0);  // 0->SPD
  TBranch *branchD = digits->GetBranch("ITSDigitsSPD");
  if (!branchD) { 
    AliError("can't get the branch with the SPD ITS digits !");
    return rv;
  }
  static TClonesArray statDigits("AliITSdigitSPD");
  TClonesArray *iITSdigits = &statDigits;
  branchD->SetAddress(&iITSdigits);  
  Int_t nDigitsL1=0;
  Int_t nDigitsL2=0;
  
  for (Int_t imod=0; imod<240; ++imod){
    digits->GetEvent(imod);
    Int_t ndigits = iITSdigits->GetEntries();
    if (imod<80) {
      fAliITSQADataMakerRec->GetDigitsData(0+fGenDigitsOffset)->Fill(0.5,ndigits);
      fAliITSQADataMakerRec->GetDigitsData(1+fGenDigitsOffset)->Fill(imod,ndigits);
      nDigitsL1+=ndigits;
    }
    else {
      fAliITSQADataMakerRec->GetDigitsData(0+fGenDigitsOffset)->Fill(1,ndigits);
      fAliITSQADataMakerRec->GetDigitsData(2+fGenDigitsOffset)->Fill(imod,ndigits);
      nDigitsL2+=ndigits;
    }
    for (Int_t idig=0; idig<ndigits; ++idig) {
      AliITSdigit *dig=(AliITSdigit*)iITSdigits->UncheckedAt(idig);
      Int_t col=dig->GetCoord1();  // cell number z
      Int_t row=dig->GetCoord2();  // cell number x
      fAliITSQADataMakerRec->GetDigitsData(3+fGenDigitsOffset)->Fill(col);
      fAliITSQADataMakerRec->GetDigitsData(4+fGenDigitsOffset)->Fill(row);
    }
  }
  fAliITSQADataMakerRec->GetDigitsData(5+fGenDigitsOffset)->Fill(nDigitsL1);
  fAliITSQADataMakerRec->GetDigitsData(6+fGenDigitsOffset)->Fill(nDigitsL2);
  fAliITSQADataMakerRec->GetDigitsData(7+fGenDigitsOffset)->Fill(nDigitsL1,nDigitsL2);
  return rv ; 
}

//____________________________________________________________________________ 
Int_t AliITSQASPDDataMakerRec::InitRecPoints()
{
  // Initialization for RECPOINTS - SPD -
  const Bool_t expert   = kTRUE ; 
  const Bool_t image    = kTRUE ; 
  Int_t rv = 0 ; 
//  fGenRecPointsOffset = (fAliITSQADataMakerRec->fRecPointsQAList[AliRecoParam::kDefault])->GetEntries();
  TH1F* hlayer= new TH1F("SPDLayPattern_SPD","Layer map - SPD",6,0.,6.);
  hlayer->GetXaxis()->SetTitle("Layer number");
  hlayer->GetYaxis()->SetTitle("Entries");
  rv = fAliITSQADataMakerRec->Add2RecPointsList(hlayer, 0+fGenRecPointsOffset, expert, !image); 
  fSPDhRecPointsTask++;

  TH1F** hmod = new TH1F*[2];
  TH1F** hxl  = new TH1F*[2];
  TH1F** hzl  = new TH1F*[2];
  TH1F** hxg  = new TH1F*[2];
  TH1F** hyg  = new TH1F*[2];
  TH1F** hzg  = new TH1F*[2];
  TH1F** hr   = new TH1F*[2];
  TH1F** hphi = new TH1F*[2];
  TH1F** hMultSPDcl = new TH1F*[2];
  TH2F** hNyNz = new TH2F*[2];  // y and z cluster length
  TH2F** hPhiZ = new TH2F*[2];

  Float_t xlim[2]={4.5,8.};
  Float_t zlim[2]={15.,15.};

  Char_t name[50];
  Char_t title[50];
  for (Int_t iLay=0;iLay<2;iLay++) {
    sprintf(name,"SPDModPattern_SPD%d",iLay+1);
    sprintf(title,"Module map - SPD Layer %d",iLay+1);
    hmod[iLay]=new TH1F(name,title,fgknSPDmodules,0,fgknSPDmodules);
    hmod[iLay]->GetXaxis()->SetTitle("Module number");
    hmod[iLay]->GetYaxis()->SetTitle("Entries");
    rv = fAliITSQADataMakerRec->Add2RecPointsList(hmod[iLay], 1+(10*iLay)+fGenRecPointsOffset, expert, !image); 
    fSPDhRecPointsTask++;

    sprintf(name,"SPDxLoc_SPD%d",iLay+1);
    sprintf(title,"Local x coordinate - SPD Layer %d",iLay+1);
    hxl[iLay]=new TH1F(name,title,100,-4.,4.);
    hxl[iLay]->GetXaxis()->SetTitle("Local x [cm]");
    hxl[iLay]->GetYaxis()->SetTitle("Entries");
    rv = fAliITSQADataMakerRec->Add2RecPointsList(hxl[iLay], 2+(10*iLay)+fGenRecPointsOffset, expert, !image);
    fSPDhRecPointsTask++;

    sprintf(name,"SPDzLoc_SPD%d",iLay+1);
    sprintf(title,"Local z coordinate - SPD Layer %d",iLay+1);
    hzl[iLay]=new TH1F(name,title,100,-4.,4.);
    hzl[iLay]->GetXaxis()->SetTitle("Local z [cm]");
    hzl[iLay]->GetYaxis()->SetTitle("Entries");
    rv = fAliITSQADataMakerRec->Add2RecPointsList(hzl[iLay], 3+(10*iLay)+fGenRecPointsOffset, expert, !image); 
    fSPDhRecPointsTask++;

    sprintf(name,"SPDxGlob_SPD%d",iLay+1);
    sprintf(title,"Global x coordinate - SPD Layer %d",iLay+1);
    hxg[iLay]=new TH1F(name,title,100,-xlim[iLay],xlim[iLay]);
    hxg[iLay]->GetXaxis()->SetTitle("Global x [cm]");
    hxg[iLay]->GetYaxis()->SetTitle("Entries");
    rv = fAliITSQADataMakerRec->Add2RecPointsList(hxg[iLay],4+(10*iLay)+fGenRecPointsOffset, expert, !image);  
    fSPDhRecPointsTask++;

    sprintf(name,"SPDyGlob_SPD%d",iLay+1);
    sprintf(title,"Global y coordinate - SPD Layer %d",iLay+1);
    hyg[iLay]=new TH1F(name,title,100,-xlim[iLay],xlim[iLay]);
    hyg[iLay]->GetXaxis()->SetTitle("Global y [cm]");
    hyg[iLay]->GetYaxis()->SetTitle("Entries");
    rv = fAliITSQADataMakerRec->Add2RecPointsList(hyg[iLay], 5+(10*iLay)+fGenRecPointsOffset, expert, !image); 
    fSPDhRecPointsTask++;

    sprintf(name,"SPDzGlob_SPD%d",iLay+1);
    sprintf(title,"Global z coordinate - SPD Layer %d",iLay+1);
    hzg[iLay]=new TH1F(name,title,150,-zlim[iLay],zlim[iLay]);
    hzg[iLay]->GetXaxis()->SetTitle("Global z [cm]");
    hzg[iLay]->GetYaxis()->SetTitle("Entries");
    rv = fAliITSQADataMakerRec->Add2RecPointsList(hzg[iLay], 6+(10*iLay)+fGenRecPointsOffset, expert, !image); 
    fSPDhRecPointsTask++;

    sprintf(name,"SPDr_SPD%d",iLay+1);
    sprintf(title,"Radius - SPD Layer %d",iLay+1);
    hr[iLay]=new TH1F(name,title,100,0.,10.);
    hr[iLay]->GetXaxis()->SetTitle("r [cm]");
    hr[iLay]->GetYaxis()->SetTitle("Entries");
    rv = fAliITSQADataMakerRec->Add2RecPointsList(hr[iLay], 7+(10*iLay)+fGenRecPointsOffset, expert, !image);  
    fSPDhRecPointsTask++;

    sprintf(name,"SPDphi_SPD%d",iLay+1);
    sprintf(title,"#varphi - SPD Layer %d",iLay+1);
    hphi[iLay]=new TH1F(name,title,1000,0.,2*TMath::Pi());
    hphi[iLay]->GetXaxis()->SetTitle("#varphi [rad]");
    hphi[iLay]->GetYaxis()->SetTitle("Entries");
    rv = fAliITSQADataMakerRec->Add2RecPointsList(hphi[iLay], 8+(10*iLay)+fGenRecPointsOffset, expert, !image);
    fSPDhRecPointsTask++;
    
    sprintf(name,"SPDSizeYvsZ_SPD%d",iLay+1);
    sprintf(title,"Cluster dimension - SPD Layer %d",iLay+1);
    hNyNz[iLay]=new TH2F(name,title,100,0.,100.,100,0.,100.);
    hNyNz[iLay]->GetXaxis()->SetTitle("z length");
    hNyNz[iLay]->GetYaxis()->SetTitle("y length");
    rv = fAliITSQADataMakerRec->Add2RecPointsList(hNyNz[iLay], 9+(10*iLay)+fGenRecPointsOffset, expert, !image); 
    fSPDhRecPointsTask++;

    sprintf(name,"SPDphi_z_SPD%d",iLay+1);
    sprintf(title,"#varphi vs z - SPD Layer %d",iLay+1);
    hPhiZ[iLay]=new TH2F(name,title,150,-zlim[iLay],zlim[iLay],200,0.,2*TMath::Pi());
    hPhiZ[iLay]->GetXaxis()->SetTitle("Global z [cm]");
    hPhiZ[iLay]->GetYaxis()->SetTitle("#varphi [rad]");
    rv = fAliITSQADataMakerRec->Add2RecPointsList(hPhiZ[iLay], 10+(10*iLay)+fGenRecPointsOffset, !expert, image);
    fSPDhRecPointsTask++;

  }

  TH2F *hrPhi=new TH2F("SPDr_phi_SPD","#varphi vs r - SPD",100,0.,10.,100,0.,2*TMath::Pi());
  hrPhi->GetXaxis()->SetTitle("r [cm]");
  hrPhi->GetYaxis()->SetTitle("#varphi [rad]");
  rv = fAliITSQADataMakerRec->Add2RecPointsList(hrPhi, 21+fGenRecPointsOffset, expert, !image);
  fSPDhRecPointsTask++;

  TH2F *hxy=new TH2F("SPDx_y_SPD","Global y vs x - SPD",200,-10.,10.,200,-10.,10.);
  hxy->GetXaxis()->SetTitle("Global x [cm]");
  hxy->GetYaxis()->SetTitle("Global y [cm]");
  rv = fAliITSQADataMakerRec->Add2RecPointsList(hxy, 22+fGenRecPointsOffset, !expert, image);
  fSPDhRecPointsTask++;

  for (Int_t iLay=0;iLay<2;iLay++) {
    sprintf(name,"SPDMultiplicity_SPD%d",iLay+1);
    sprintf(title,"Cluster multiplicity - SPD Layer %d",iLay+1);
    hMultSPDcl[iLay]=new TH1F(name,title,200,0.,200.);
    hMultSPDcl[iLay]->GetXaxis()->SetTitle("Cluster multiplicity");
    hMultSPDcl[iLay]->GetYaxis()->SetTitle("Entries");
    rv = fAliITSQADataMakerRec->Add2RecPointsList(hMultSPDcl[iLay], 23+iLay+fGenRecPointsOffset, !expert, image);
    fSPDhRecPointsTask++;
  } 

  TH2F *hMultSPDcl2MultSPDcl1 =
        new TH2F("SPDMultCorrelation_SPD","Cluster multiplicity correlation - SPD",200,0.,200.,200,0.,200.);
  hMultSPDcl2MultSPDcl1->GetXaxis()->SetTitle("Clusters multiplicity (Layer 1)");
  hMultSPDcl2MultSPDcl1->GetYaxis()->SetTitle("Clusters multiplicity (Layer 2)"); 
  rv = fAliITSQADataMakerRec->Add2RecPointsList(hMultSPDcl2MultSPDcl1, 25+fGenRecPointsOffset, !expert, image);
  fSPDhRecPointsTask++;

  AliDebug(AliQAv1::GetQADebugLevel(),Form("%d SPD Recs histograms booked\n",fSPDhRecPointsTask));

  return rv ; 
}

//____________________________________________________________________________ 
Int_t AliITSQASPDDataMakerRec::MakeRecPoints(TTree * clusterTree)
{
  // Fill QA for RecPoints - SPD -
  Int_t rv = 0 ; 
  static TClonesArray statITSCluster("AliITSRecPoint");
  TClonesArray *ITSCluster = &statITSCluster;
  TBranch* itsClusterBranch=clusterTree->GetBranch("ITSRecPoints");
  if (!itsClusterBranch) {
    AliError("can't get the branch with the ITS clusters !");
    return rv;
  }
  // Check id histograms already created for this Event Specie
  if ( ! fAliITSQADataMakerRec->GetRecPointsData(fGenRecPointsOffset) )
    rv = InitRecPoints() ;
  
  itsClusterBranch->SetAddress(&ITSCluster);
  Int_t nItsMods = (Int_t)clusterTree->GetEntries();
  
  Float_t cluGlo[3] = {0.,0.,0.};
  Int_t nClusters[2] = {0,0};
  
  for (Int_t iIts=0; iIts < nItsMods; iIts++) {
    
    if (!clusterTree->GetEvent(iIts))    continue;
    Int_t nCluster = ITSCluster->GetEntriesFast();
    // loop over clusters
    while(nCluster--) {
      AliITSRecPoint* cluster = (AliITSRecPoint*)ITSCluster->UncheckedAt(nCluster);
      
      if (cluster->GetLayer()>1)        continue;
      Int_t lay=cluster->GetLayer();
      fAliITSQADataMakerRec->GetRecPointsData(0 +fGenRecPointsOffset)->Fill(lay);
      cluster->GetGlobalXYZ(cluGlo);
      Float_t rad=TMath::Sqrt(cluGlo[0]*cluGlo[0]+cluGlo[1]*cluGlo[1]);
        Float_t phi= TMath::Pi() + TMath::ATan2(-cluGlo[1],-cluGlo[0]);
        if (lay==0) {
	  fAliITSQADataMakerRec->GetRecPointsData(1 +fGenRecPointsOffset)->Fill(iIts);
	  fAliITSQADataMakerRec->GetRecPointsData(2 +fGenRecPointsOffset)->Fill(cluster->GetDetLocalX());
	  fAliITSQADataMakerRec->GetRecPointsData(3 +fGenRecPointsOffset)->Fill(cluster->GetDetLocalZ());
	  fAliITSQADataMakerRec->GetRecPointsData(4 +fGenRecPointsOffset)->Fill(cluGlo[0]);
	  fAliITSQADataMakerRec->GetRecPointsData(5 +fGenRecPointsOffset)->Fill(cluGlo[1]);
	  fAliITSQADataMakerRec->GetRecPointsData(6 +fGenRecPointsOffset)->Fill(cluGlo[2]);
	  fAliITSQADataMakerRec->GetRecPointsData(7 +fGenRecPointsOffset)->Fill(rad);
	  fAliITSQADataMakerRec->GetRecPointsData(8 +fGenRecPointsOffset)->Fill(phi);
	  fAliITSQADataMakerRec->GetRecPointsData(9 +fGenRecPointsOffset)->Fill(cluster->GetNz(),cluster->GetNy());
	  fAliITSQADataMakerRec->GetRecPointsData(10 +fGenRecPointsOffset)->Fill(cluGlo[2],phi);
 	} else  {
          fAliITSQADataMakerRec->GetRecPointsData(11 +fGenRecPointsOffset)->Fill(iIts);
          fAliITSQADataMakerRec->GetRecPointsData(12 +fGenRecPointsOffset)->Fill(cluster->GetDetLocalX());
          fAliITSQADataMakerRec->GetRecPointsData(13 +fGenRecPointsOffset)->Fill(cluster->GetDetLocalZ());
          fAliITSQADataMakerRec->GetRecPointsData(14 +fGenRecPointsOffset)->Fill(cluGlo[0]);
          fAliITSQADataMakerRec->GetRecPointsData(15 +fGenRecPointsOffset)->Fill(cluGlo[1]);
          fAliITSQADataMakerRec->GetRecPointsData(16 +fGenRecPointsOffset)->Fill(cluGlo[2]);
          fAliITSQADataMakerRec->GetRecPointsData(17 +fGenRecPointsOffset)->Fill(rad);
          fAliITSQADataMakerRec->GetRecPointsData(18 +fGenRecPointsOffset)->Fill(phi);
          fAliITSQADataMakerRec->GetRecPointsData(19 +fGenRecPointsOffset)->Fill(cluster->GetNz(),cluster->GetNy());
          fAliITSQADataMakerRec->GetRecPointsData(20 +fGenRecPointsOffset)->Fill(cluGlo[2],phi);
        }
        fAliITSQADataMakerRec->GetRecPointsData(21 +fGenRecPointsOffset)->Fill(rad,phi);
        fAliITSQADataMakerRec->GetRecPointsData(22 +fGenRecPointsOffset)->Fill(cluGlo[0],cluGlo[1]);
	
	nClusters[lay]++; 
    } // end of cluster loop
  } // end of its "subdetector" loop
  
  for (Int_t iLay=0; iLay<2; iLay++)
    fAliITSQADataMakerRec->GetRecPointsData(23+iLay +fGenRecPointsOffset)->Fill(nClusters[iLay]);
  
  fAliITSQADataMakerRec->GetRecPointsData(25 +fGenRecPointsOffset)->Fill(nClusters[0],nClusters[1]);
  
  statITSCluster.Clear();
  return rv ;
}



//_______________________________________________________________

Int_t AliITSQASPDDataMakerRec::GetOffset(AliQAv1::TASKINDEX_t task) {
  // Returns offset number according to the specified task
  Int_t offset=0;
  if( task == AliQAv1::kRAWS ) {
    offset=fGenRawsOffset;
  }
  else if( task == AliQAv1::kDIGITSR ) {
    offset=fGenDigitsOffset;
  }
  else if( task == AliQAv1::kRECPOINTS ) {
    offset=fGenRecPointsOffset;
  }
  else if( task == AliQAv1::kDIGITS ) {
    offset=fGenDigitsOffset;
  }
  else {
    AliInfo("No task has been selected. Offset set to zero.\n");
  }

  return offset;
}

//_______________________________________________________________

void AliITSQASPDDataMakerRec::SetOffset(AliQAv1::TASKINDEX_t task, Int_t offset) {
  // Returns offset number according to the specified task
  if( task == AliQAv1::kRAWS ) {
   fGenRawsOffset=offset;
  }
  else if( task == AliQAv1::kDIGITSR ) {
    fGenDigitsOffset=offset;
  }
  else if( task == AliQAv1::kRECPOINTS ) {
    fGenRecPointsOffset=offset;
  }
   else {
    AliInfo("No task has been selected. Offset set to zero.\n");
  }
}

//_______________________________________________________________

Int_t AliITSQASPDDataMakerRec::GetTaskHisto(AliQAv1::TASKINDEX_t task) {
  // Returns the number of histograms associated to the specified task

  Int_t histotot=0;

  if( task == AliQAv1::kRAWS ) {
    histotot=fSPDhRawsTask;
  }
  else if( task == AliQAv1::kDIGITSR ) {
    histotot=fSPDhDigitsTask;
  }
  else if( task == AliQAv1::kRECPOINTS ){
    histotot=fSPDhRecPointsTask;
  }
  else if( task == AliQAv1::kDIGITS ){
    histotot=fSPDhDigitsTask;
  }
  else {
    AliInfo("No task has been selected. TaskHisto set to zero.\n");
  }
  return histotot;
}
