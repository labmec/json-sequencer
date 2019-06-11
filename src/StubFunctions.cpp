//
// Created by gus on 04/02/19.
//

#include "StubFunctions.h"
#include "pzstack.h"
#include "telasticmodel.h"

#include "TElasticAdjust.h"
#include "TF2DSAdjust_DW.h"




TGlob glob;

void open(std::string filePath, std::string nickname) {
    std::string complete = "../Inputdata/"+filePath;
    std::cout << "open(" << complete << ", " << nickname << ");" << std::endl;
    TTestData data(complete,nickname);
    glob.fMeasure[nickname] = data;
}

void model(std::string modelName, std::string test) {
    std::cout << "model(" << modelName << ", " << test << ");" << std::endl;
    if(modelName.compare("elasticity") == 0)
    {
        glob.fAdjust = EElasticResponse;
    } else
    if(modelName.compare("CapDSplasticity") == 0)
    {
        glob.fAdjust = DiMaggioSandlerF2Response;
    }
}

void select(std::string parameter, int initial, int final, std::string comment) {
    std::cout << "select(" << parameter << ", " << initial << ", " << final << ", " << comment << ");" << std::endl;
    if(glob.fMeasure.find(parameter) == glob.fMeasure.end())
    {
        std::cout << "SELECTION NOT FOUND ******\n";
    }
    else
    {
        TTestData *test = &glob.fMeasure[parameter];
        TTestSection selected(test,initial,final,comment);
        glob.fActive.Push(selected);
    }
}

void adjust(std::string parameter) {
    std::cout << "adjust(" << parameter << ");" << std::endl;
    if(glob.fAdjust == EElasticResponse)
    {
        TElasticAdjust model;
        model.Adjust(glob.fActive);
        glob.fER = model.ElasticResponse();
        {
            int nactive = glob.fActive.size();
            std::ofstream out("Adjust.nb");
            for(int i=0; i<nactive; i++)
            {
                TPZFMatrix<REAL> deform,stress_meas,stress_comp;
                glob.fActive[i].GetData(deform,stress_meas);
                model.ComputedSigma(glob.fActive[i],i,stress_comp);
                deform.Print("Deform = ",out,EMathematicaInput);
                stress_meas.Print("StressMeas = ",out,EMathematicaInput);
                stress_comp.Print("StressComp = ",out,EMathematicaInput);
            }
        }
    }
    if(glob.fAdjust == DiMaggioSandlerF2Response)
    {
        
        TF2DSAdjust_DW model;
        model.PopulateElastic();
        model.PlotXvsEpsPv(glob.fActive);
                
        {
            int nactive = glob.fActive.size();
            std::ofstream out("Adjust.nb");
            for(int i=0; i<nactive; i++)
            {
                TPZFMatrix<REAL> deform,stress_meas,stress_comp,invariantStress;
                glob.fActive[i].GetData(deform,stress_meas);
                glob.fActive[i].GetInvStressData(invariantStress);
                
                deform.Print("Deform = ",out,EMathematicaInput);
                invariantStress.Print("InvariantStress = ",out,EMathematicaInput);
                
            }
        }
    }
    else
    {
        std::cout << "I dont know what to adjust *****\n";
    }
}

void assign(std::string parameter, std::string fileNickname) {
    std::cout << "assign(" << parameter << ", " << fileNickname << ");" << std::endl;
}

void clear() {
    std::cout << "clear();" << std::endl;
}

TTestData::TTestData(std::string &filename, std::string &nickname) : fNickName(nickname), fFileName(filename)
{
    int64_t numdata = NumData();
    fDeform.Redim(numdata,2);
    fStress.Redim(numdata,2);
    fInvariantStress.Redim(numdata,2);
    std::ifstream input(filename);
    ReadHeader(input);
    for(int64_t i = 0; i<numdata; i++)
    {
        ReadLine(i,input);
    }

}
//TPZFMatrix<double> fDeform;
//TPZFMatrix<double> fStress;

//std::string fNickName;

//std::string fFileName;

//TPZVec<EDataType> fTypes;

//// 0 -> sigc
//// 1 -> siga
//// 2 -> epsc
//// 3 -> epsa
//std::map<int,int> fRelevant;



TTestData::TTestData() : fDeform(), fStress(), fInvariantStress(),fNickName(), fFileName(), fTypes(), fRelevant()
{

}

TTestData::TTestData(const TTestData &cp) : fDeform(cp.fDeform), fStress(cp.fStress), fInvariantStress(cp.fInvariantStress), fNickName(cp.fNickName), fFileName(cp.fFileName), fTypes(cp.fTypes), fRelevant(cp.fRelevant)
{

}

TTestData &TTestData::operator=(const TTestData &cp)
{
    fDeform   = cp.fDeform;
    fStress   = cp.fStress;
    fInvariantStress = cp.fInvariantStress;
    fNickName = cp.fNickName;
    fFileName = cp.fFileName;
    fTypes    = cp.fTypes;
    fRelevant = cp.fRelevant;
    
    return *this;
}


void TTestData::GetData(int64_t first, int64_t last, TPZFMatrix<double> &deform, TPZFMatrix<double> &stress)
{
    deform.Redim(last-first+1,2);
    stress.Redim(last-first+1,2);
    for(int64_t i=first; i<=last; i++)
    {
        deform(i-first,0) = - fDeform(i,0)*0.01;
        deform(i-first,1) = - fDeform(i,1)*0.01;
        stress(i-first,0) = - fStress(i,0);
        stress(i-first,1) = - (this->fStress(i,1));
    }
    
    
}


void TTestData::GetInvStressData(int64_t first, int64_t last, TPZFMatrix<double> &invariantStress)
{
    invariantStress.Redim(last-first+1,2);
    for(int64_t i=first; i<=last; i++)
    {
        invariantStress(i-first,0) = - fInvariantStress(i,0);
        invariantStress(i-first,1) = this->fInvariantStress(i,1);
        
    }
}


void TTestData::ReadHeader(std::istream &input)
{
    std::string headerline;
    std::getline(input,headerline);
    TPZStack<std::string> titles;
    std::stringstream strstream(headerline);
    while(strstream)
    {
        std::string loc;
        strstream >> loc;
        if(loc.size()) titles.Push(loc);
    }
    this->fTypes.resize(titles.size());
    //Time	siga	epsc	epsa	epsv	sigc	siga*	SqJ2	I1

    for(int i=0; i<titles.size(); i++)
    {
        if(titles[i].compare("Time") == 0){
            fTypes[i] = ETime;
        }
        else if(titles[i].compare("siga") == 0)
        {
            fTypes[i] = ESiga;
        }
        else if(titles[i].compare("epsc") == 0)
        {
            fTypes[i] = EEpsr;
            fRelevant[2] = i;
        }
        else if(titles[i].compare("epsa") == 0)
        {
            fTypes[i] = ESiga;
            fRelevant[3] = i;
        }
        else if(titles[i].compare("epsv") == 0)
        {
            fTypes[i] = EEpsv;
        }
        else if(titles[i].compare("sigc") == 0)
        {
            fTypes[i] = ESigr;
            fRelevant[0] = i;
        }
        else if(titles[i].compare("siga*") == 0)
        {
            fTypes[i] = ESigaStar;
            fRelevant[1] = i;
        }
        else if(titles[i].compare("SqJ2") == 0)
        {
            fTypes[i] = ESqJ2;
            fRelevant[5] = i;
        }
        else if(titles[i].compare("I1") == 0)
        {
            fTypes[i] = EI1;
            fRelevant[4] = i;
        }
        else
        {
            std::cout << "I dont recognize " << titles[i] << std::endl;
        }
    }
    if(fRelevant.size() != 6)
    {
        std::cout << "Didnt find all relevant titles\n";
    }
    else
    {
        for(auto it = fRelevant.begin(); it != fRelevant.end(); it++)
        {
            std::cout << "mapping index " << it->first << " to " << it->second << std::endl;
        }
    }
}

void TTestData::ReadLine(std::int64_t index, std::istream &input)
{
    std::string headerline;
    std::getline(input,headerline);
    TPZStack<REAL> values;
    std::stringstream strstream(headerline);
    for(int i=0; i< fTypes.size(); i++)
    {
        REAL val;
        strstream >> val;
        values.Push(val);
    }
    fStress(index,0)  = values[fRelevant[0]];
    fStress(index,1)  = values[fRelevant[1]];
    fDeform(index,0)  = values[fRelevant[2]];
    fDeform(index,1)  = values[fRelevant[3]];
    fInvariantStress(index,0) = values[fRelevant[4]];
    fInvariantStress(index,1) = values[fRelevant[5]];
}

/// computes the number of lines in the file
int64_t TTestData::NumData()
{
    std::ifstream input(fFileName);
    std::string line;
    int64_t counter = 0;
    std::getline(input,line);
    if(!input)
    {
        std::cout << __PRETTY_FUNCTION__ << " Corrupted file\n";
        return -1;
    }
    while(input)
    {
        std::getline(input,line);
        if(input && line.size() > 0) counter++;
    }
    return counter;
}

