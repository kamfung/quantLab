//
//  tradeMgr.cpp
//  Created on 27/9/2018.
//
//  To compile: g++ -o tradeMgr -std=c++11 tradeMgr.cpp
//  To run: ./tradeMgr input.csv output.cvs
//
//  Copyright © 2018 Xiliang.Liu. All rights reserved.
//

#include <iostream>
#include <sstream>
#include <regex>
#include <fstream>
#include <string>
#include <map>

using namespace std;

/*
 * Used to read data entry from the input file
 */
struct TDataEntry {
    long    timestamp;
    string  instrument;
    double  quantity;
    double  price;
    void print() const {
        cout << timestamp << "," << instrument << ","
             << quantity << "," << price << endl;
    }
};

/*
 * Used to stored relavent trade data for each instrument
 */
struct TInstTradeData {
    long    lastTradeTime;
    long    maxTradeGap;
    double  totalVolume;
    double  totalConsideration;
    double  maxPrice;
    TInstTradeData(){};
    TInstTradeData(const TDataEntry &entry):
        lastTradeTime(entry.timestamp),
        maxTradeGap(0),
        totalVolume(entry.quantity),
        totalConsideration(entry.price * entry.quantity),
        maxPrice(entry.price) {}
};

/*
 * This functor defines an interface based on which different derived functors can be
 * defined, allowing for future extensibility to present the data in different ways.
 */
class CDataPresenter {
public:
    virtual void operator()( ostream & out, const map<string,TInstTradeData> &data) = 0;
};

class CQuantLabPresenter : public CDataPresenter {
public:
    virtual void operator() (ostream & out, const map<string, TInstTradeData> &data) {
        for(auto &p: data) {
            out << p.first << "," << p.second.maxTradeGap << ","
            << p.second.totalVolume << ","
            << long(p.second.totalConsideration / p.second.totalVolume) << ","
            << p.second.maxPrice << endl;
        }
    }
};

/*
 * A class that processes incoming trade data entries, stores relavent trade data
 * information for easy lookup and presentation based on instruments.
 */
class CTradeDataMgr {
public:
    void addDataEntry(const TDataEntry & entry);
    void setDataPresenter(CDataPresenter* p);
    void presentData(ostream &out);
    
private:
    map<string,TInstTradeData> tradeDataLookup;
    CDataPresenter *presenter = nullptr;
};

void CTradeDataMgr::addDataEntry(const TDataEntry &entry)
{
    if (tradeDataLookup.find(entry.instrument) == tradeDataLookup.end()) {
        TInstTradeData tradeData (entry);
        tradeDataLookup[entry.instrument] = tradeData;
    } else {
        auto & pdata = tradeDataLookup[entry.instrument];
        pdata.maxTradeGap = max(pdata.maxTradeGap,
                                entry.timestamp - pdata.lastTradeTime);
        pdata.maxPrice = max(pdata.maxPrice, entry.price);
        pdata.lastTradeTime = entry.timestamp;
        pdata.totalVolume += entry.quantity;
        pdata.totalConsideration += entry.price * entry.quantity;
    }
}

void CTradeDataMgr::setDataPresenter(CDataPresenter* p)
{
    presenter = p;
}

void CTradeDataMgr::presentData(ostream & out)
{
    if (presenter != nullptr) {
        (*presenter)(out, tradeDataLookup);
    }
}

/*
 * This class is used to read data entry from an input file.
 */
class CDataIterator {
public:
    bool HasNext();
    TDataEntry GetNext();
    CDataIterator(ifstream & input, const string &f):dataStream(input), format(f){};
    
private:
    ifstream & dataStream;
    string line;
    regex format;
};

/*
 * Check if there is a valid line in the file to read
 */
bool CDataIterator::HasNext()
{
    while (getline(dataStream, line, '\n')) {
        //remove the trailing return char if the give input file is in dos format
        if (line.back() == '\r'){
            line.pop_back();
        }
        if (regex_match(line, format)) {
            return true;
        } else {
            clog << "Skipping mal-formatted data line : " << line << endl;
        }
    }
    return false;
}

/*
 * Process the next line from input into a data entry
 */
TDataEntry CDataIterator::GetNext()
{
    vector<string> fields;
    stringstream ss(line);
    string field;
    
    while (getline(ss, field, ',')) {
        fields.push_back(field);
    }
    
    TDataEntry res = {stol(fields[0]),
                      fields[1],
                      stod (fields[2]),
                      stod (fields[3])};
    return res;
}

int main(int argc, const char * argv[]) {
    if (argc != 3) {
        cout << "Usage: " << argv[0] << " <input_file>" << " <output_file>" << endl;
        return -1;
    }
    
    ifstream    input(argv[1]);
    ofstream    output(argv[2]);
    if (!input.good()) {
        cerr << "Failed to open file: " << argv[1] << endl;
        return -1;
    }
    if (!output.good()) {
        cerr << "Failed to open output file: " << argv[2] << endl;
        return -1;
    }
    
    const string dataFormat = "[0-9]+,[a-z]+,[0-9]+,[0-9]+";
    CDataIterator dataIt(input, dataFormat);
    CQuantLabPresenter qLabPresenter;
    CTradeDataMgr tradeMgr;
    tradeMgr.setDataPresenter(&qLabPresenter);
    
    while (dataIt.HasNext()) {
        TDataEntry dataEntry = dataIt.GetNext();
        tradeMgr.addDataEntry(dataEntry);
    }
    tradeMgr.presentData(output);
    
    return 0;
}
