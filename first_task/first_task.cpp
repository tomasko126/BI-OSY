#ifndef __PROGTEST__

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <climits>
#include <cfloat>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <vector>
#include <set>
#include <list>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <stack>
#include <deque>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include "progtest_solver.h"
#include "sample_tester.h"

using namespace std;
#endif /* __PROGTEST__ */

class CWeldingCompany {
public:
    static void SeqSolve (APriceList priceList, COrder &order) {
        vector<COrder> orders;
        orders.push_back(order);

        ProgtestSolver(orders, priceList);

        cout << orders[0].m_Cost << endl;
    }

    void AddProducer (AProducer prod) {

    }

    void AddCustomer (ACustomer cust) {

    }

    void AddPriceList (AProducer prod, APriceList priceList) {

    }

    void Start (unsigned thrCount) {

    }

    void Stop (void) {

    }
private:
    
};

//-------------------------------------------------------------------------------------------------
#ifndef __PROGTEST__

int main(void) {
    using namespace std::placeholders;
    CWeldingCompany m;

    CProd prod(6, 7, 1);

    shared_ptr<CPriceList> list = make_shared<CPriceList>(1);
    list->Add(prod);

    COrder order(12, 14, 0);

    m.SeqSolve(list, order);

    CWeldingCompany test;

    AProducer p1 = make_shared<CProducerSync>(bind(&CWeldingCompany::AddPriceList, &test, _1, _2));
    AProducerAsync p2 = make_shared<CProducerAsync>(bind(&CWeldingCompany::AddPriceList, &test, _1, _2));
    test.AddProducer(p1);
    test.AddProducer(p2);
    test.AddCustomer(make_shared<CCustomerTest>(2));
    p2->Start();
    test.Start(3);
    test.Stop();
    p2->Stop();
    return 0;
}

#endif /* __PROGTEST__ */
