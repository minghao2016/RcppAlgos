#include <Rcpp.h>
#include <math.h>
#include <stdint.h>
#include "PollardRho.h"
using namespace Rcpp;

const double Significand53 = 9007199254740991.0;

template <typename typeInt>
inline typeInt getStartingIndex (typeInt lowerB, typeInt step) {
    
    typeInt retStrt, remTest = lowerB % step;
    
    if (remTest == 0) {
        retStrt = 0;
    } else if (step < lowerB) {
        retStrt = step - remTest;
    } else {
        retStrt = step - lowerB;
    }
    
    return retStrt;
}

template <typename typeRcpp, typename typeInt>
typeRcpp NumDivisorsSieve (typeInt m, typeInt n, bool keepNames) {
    
    typeInt myRange = n;
    myRange += (1 - m);
    std::vector<typeInt> myNames;
    
    typeInt myNum = m;
    
    if (keepNames){
        myNames.resize(myRange);
        for (std::size_t k = 0, j = myNum; j <= n; j++, k++)
            myNames[k] = j;
    }
    
    std::vector<typeInt> numFacs;
    typeInt i, j;
    
    if (m < 2) {
        numFacs = std::vector<typeInt> (myRange, 1);
        for (i = 2; i <= n; i++)
            for (j = i; j <= n; j+=i)
                numFacs[j - 1]++;
    } else {
        numFacs = std::vector<typeInt> (myRange, 2);
        int_fast32_t sqrtBound = floor(sqrt((double)n));
        typeInt myStart, testNum;
        
        for (i = 2; i <= sqrtBound; i++) {
            myStart = getStartingIndex(m, i);
            myNum = m + myStart;
            
            for (j = myStart; j < myRange; j += i, myNum += i) {
                numFacs[j]++;
                testNum = myNum / i;
                if (testNum > sqrtBound)
                    numFacs[j]++;
            }
        }
    }
    
    typeRcpp myVector = wrap(numFacs);
    if (keepNames)
        myVector.attr("names") = myNames;
    
    return myVector;
}

template <typename typeInt>
List DivisorListRcpp (typeInt m, typeInt n, bool keepNames) {
    
    typeInt myRange = n;
    myRange += (1 - m);
    
    std::vector<std::vector<typeInt> > MyDivList(myRange, std::vector<typeInt>(1, 1));
    typename std::vector<std::vector<typeInt> >::iterator it2d, itEnd;
    itEnd = MyDivList.end();
    // Most values will have fewer than 2 times the 
    // maximal number of bits in m (crude analysis).
    // We don't want to consider the few highly
    // composite values when determing our memory
    // reservation as that will allocate way more
    // memory than necessary for the majority of values.
    typeInt i, j, myMalloc = 2*ceil(log2((double) n)), myNum = m;
    std::vector<typeInt> myNames;
    
    if (keepNames){
        myNames.resize(myRange);
        for (std::size_t k = 0, j = myNum; j <= n; j++, k++)
            myNames[k] = j;
    }
    
    if (m < 2) {
        for (it2d = MyDivList.begin() + 1; it2d < itEnd; it2d++)
            it2d -> reserve(myMalloc);
        
        for (i = 2; i <= n; i++)
            for (j = i; j <= n; j += i)
                MyDivList[j - 1].push_back(i);
        
    } else {
        int_fast32_t sqrtBound = floor(sqrt((double)n));
        typeInt myStart, testNum;
        
        for (it2d = MyDivList.begin(); it2d < itEnd; it2d++, myNum++) {
            it2d -> reserve(myMalloc);
            it2d -> push_back(myNum);
        }
        
        for (i = sqrtBound; i >= 2; i--) {
            
            myStart = getStartingIndex(m, i);
            myNum = m + myStart;
            
            for (j = myStart; j < myRange; j += i, myNum += i) {
                // Put element in the second position. (see comment below)
                MyDivList[j].insert(MyDivList[j].begin() + 1, i);
                testNum = myNum / i;
                
                // Ensure we won't duplicate adding an element. If
                // testNum <= sqrtBound, it will be added in later
                // iterations. Also, we insert this element in the
                // pentultimate position as it will be the second
                // to the largest element at the time of inclusion.
                // E.g. let i = 5, myNum = 100, so the current
                // vectors looks like so: v = 1, 5, 10, 100 (5 was
                // added to the second position above). With i = 5,
                // testNum = 100 / 5 = 20, thus we add it the second
                // to last position to give v = 1 5 10 20 100.
                if (testNum > sqrtBound)
                    MyDivList[j].insert(MyDivList[j].end() - 1, testNum);
            }
        }
    }
    
    Rcpp::List myList = wrap(MyDivList);
    if (keepNames)
        myList.attr("names") = myNames;
    
    return myList;
}

// [[Rcpp::export]]
SEXP DivisorsGeneral (SEXP Rb1, SEXP Rb2, 
                       SEXP RIsList, SEXP RNamed) {
    double bound1, bound2, myMax, myMin;
    bool isList = false, isNamed = false;
    
    switch(TYPEOF(Rb1)) {
        case REALSXP: {
            bound1 = as<double>(Rb1);
            break;
        }
        case INTSXP: {
            bound1 = as<double>(Rb1);
            break;
        }
        default: {
            stop("bound1 must be of type numeric or integer");
        }
    }
    
    isList = as<bool>(RIsList);
    isNamed = as<bool>(RNamed);
    
    if (Rf_isNull(Rb2)) {
        if (bound1 <= 0) {stop("n must be positive");}
        myMax = floor(bound1);
        
        if (isList) {
            if (myMax < 2) {
                std::vector<std::vector<int> > trivialRet(1, std::vector<int>(1, 1));
                Rcpp::List z = wrap(trivialRet);
                if (isNamed)
                    z.attr("names") = 1;
                
                return z;
            } else {
                if (bound1 > (INT_MAX - 1)) {
                    return DivisorListRcpp((int_fast64_t) 1,
                                                  (int_fast64_t) myMax, isNamed);
                }
                return DivisorListRcpp((int_fast32_t) 1,
                                              (int_fast32_t) myMax, isNamed);
            }
        } else {
            if (myMax < 2) {
                IntegerVector v(1, 1);
                if (isNamed)
                    v.attr("names") = 1;
                
                return v;
            } else {
                if (bound1 > (INT_MAX - 1)) {
                    return NumDivisorsSieve<NumericVector>((int_fast64_t) 1,
                                                       (int_fast64_t) myMax, isNamed);
                }
                return NumDivisorsSieve<IntegerVector>((int_fast32_t) 1,
                                                   (int_fast32_t) myMax, isNamed);
            }
        }
    } else {
        if (bound1 <= 0 || bound1 > Significand53)
            stop("bound1 must be a positive number less than 2^53");
        
        switch(TYPEOF(Rb2)) {
            case REALSXP: {
                bound2 = as<double>(Rb2);
                break;
            }
            case INTSXP: {
                bound2 = as<double>(Rb2);
                break;
            }
            default: {
                stop("bound2 must be of type numeric or integer");
            }
        }
        if (bound2 <= 0 || bound2 > Significand53)
            stop("bound2 must be a positive number less than 2^53");
        
        if (bound1 > bound2) {
            myMax = bound1;
            myMin = bound2;
        } else {
            myMax = bound2;
            myMin = bound1;
        }
        
        myMin = ceil(myMin);
        myMax = floor(myMax);
        
        if (isList) {
            if (myMax < 2) {
                std::vector<std::vector<int> > trivialRet(1, std::vector<int>(1, 1));
                Rcpp::List z = wrap(trivialRet);
                if (isNamed)
                    z.attr("names") = 1;
                
                return z;
            } else {
                if (bound1 > (INT_MAX - 1)) {
                    return DivisorListRcpp((int_fast64_t) myMin,
                                                  (int_fast64_t) myMax, isNamed);
                }
                return DivisorListRcpp((int_fast32_t) myMin,
                                              (int_fast32_t) myMax, isNamed);
            }
        } else {
            if (myMax < 2) {
                IntegerVector v(1, 1);
                if (isNamed)
                    v.attr("names") = 1;
                
                return v;
            } else {
                if (bound1 > (INT_MAX - 1)) {
                    return NumDivisorsSieve<NumericVector>((int_fast64_t) myMin,
                                                           (int_fast64_t) myMax, isNamed);
                }
                return NumDivisorsSieve<IntegerVector>((int_fast32_t) myMin,
                                                       (int_fast32_t) myMax, isNamed);
            }
        }
    }
}


std::vector<int64_t> Factorize (int64_t t, std::vector<int64_t>& factors) {
    
    if (t == 1) {
        std::vector<int64_t> trivialReturn;
        if (factors.size() > 0) {trivialReturn.push_back(factors[0]);}
        trivialReturn.push_back(1);
        return trivialReturn;
    } else {
        std::vector<unsigned long int> lengths;
        std::vector<int64_t>::iterator it, facEnd;
        facEnd = factors.end();
        int64_t prev = factors[0];
        
        unsigned long int i, j, k, n = factors.size(), numUni = 0;
        std::vector<int64_t> uniFacs(n);
        uniFacs[0] = factors[0];
        lengths.reserve(n);
        lengths.push_back(1);
        
        for(it = factors.begin() + 1; it < facEnd; it++) {
            if (prev == *it) {
                lengths[numUni]++;
            } else {
                numUni++;
                prev = *it;
                lengths.push_back(1);
                uniFacs[numUni] = (int64_t)*it;
            }
        }
        
        unsigned long int ind, facSize = 1, numFacs = 1;
        for (i = 0; i <= numUni; i++) {numFacs *= (lengths[i]+1);}
        
        std::vector<int64_t> myFacs(numFacs);
        int64_t temp;
        
        for (i = 0; i <= lengths[0]; ++i) {
            myFacs[i] = (int64_t) std::pow(uniFacs[0], i);
        }
        
        if (numUni > 0) {
            for (j = 1; j <= numUni; j++) {
                facSize *= (lengths[j-1] + 1);
                for (i = 1; i <= lengths[j]; i++) {
                    ind = i*facSize;
                    for (k = 0; k < facSize; k++) {
                        temp = (int64_t) std::pow(uniFacs[j], i);
                        temp *= myFacs[k];
                        myFacs[ind + k] = temp;
                    }
                }
            }
        }
        
        std::sort(myFacs.begin(), myFacs.end());
        return myFacs;
    }
}


// [[Rcpp::export]]
SEXP getAllDivisorsRcpp (SEXP Rv, SEXP RNamed) {
    int64_t mPass;
    std::vector<int64_t> myNums;
    bool isNegative = false, isNamed = false;
    
    switch(TYPEOF(Rv)) {
        case REALSXP: {
            myNums = as<std::vector<int64_t> >(Rv);
            break;
        }
        case INTSXP: {
            myNums = as<std::vector<int64_t> >(Rv);
            break;
        }
        default: {
            stop("v must be of type numeric or integer");
        }
    }
    
    isNamed = as<bool>(RNamed);
    unsigned int myLen = myNums.size();
    
    if (myLen > 1) {
        std::vector<std::vector<int64_t> > MyDivList(myLen, std::vector<int64_t>());
        
        for (std::size_t j = 0; j < myLen; j++) {
            std::vector<int64_t> myDivisors;
            mPass = myNums[j];
            
            if (mPass < 0) {
                mPass = std::abs(mPass);
                isNegative = true;
            } else {
                isNegative = false;
            }
            
            if (mPass > 0) {
                std::vector<int64_t> factors;
                mPass = mPass;
                getPrimefactors(mPass, factors);
                myDivisors = Factorize(mPass, factors);
                if (isNegative) {
                    unsigned int facSize = myDivisors.size();
                    std::vector<int64_t> tempInt(2 * facSize);
                    unsigned int posInd = facSize, negInd = facSize - 1;
                    
                    for (std::size_t i = 0; i < facSize; i++, posInd++, negInd--) {
                        tempInt[negInd] = -1 * myDivisors[i];
                        tempInt[posInd] = myDivisors[i];
                    }
                    
                    myDivisors = tempInt;
                }
            }
            MyDivList[j] = myDivisors;
        }
        
        Rcpp::List myList = wrap(MyDivList);
        if (isNamed)
            myList.attr("names") = myNums;
        return myList;
    } else {
        std::vector<int64_t> myDivisors;
        mPass = myNums[0];
        
        if (mPass < 0) {
            mPass = std::abs(mPass);
            isNegative = true;
        } else {
            isNegative = false;
        }
        
        if (myNums[0] > 0) {
            std::vector<int64_t> factors;
            getPrimefactors(mPass, factors);
            myDivisors = Factorize(mPass, factors);
            if (isNegative) {
                unsigned int facSize = myDivisors.size();
                std::vector<int64_t> tempInt(2 * facSize);
                unsigned int posInd = facSize, negInd = facSize - 1;
                
                for (std::size_t i = 0; i < facSize; i++, posInd++, negInd--) {
                    tempInt[negInd] = -1 * myDivisors[i];
                    tempInt[posInd] = myDivisors[i];
                }
                
                return wrap(tempInt);
            }
        }
        
        return wrap(myDivisors);
    }
}