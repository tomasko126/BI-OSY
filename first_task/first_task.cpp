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

class PriceList {
public:
    // Store products in price list
    void AddProductsToPriceList (AProducer producer, APriceList priceList) {
        unsigned int materialID = priceList->m_MaterialID;

        // Add producer into list of producers
        listOfProducers[materialID].insert(producer);

        //cout << "AddProductsToPriceList call" << endl;

        // There's no such material stored
        if (listOfPriceList.find(materialID) == listOfPriceList.end()) {

            //cout << "Inserting products into pricelist as new..." << endl;

            /*
            for (const auto & prod : priceList->m_List) {
                cout << "ID: " << priceList->m_MaterialID << " W: " << prod.m_W << " H: " << prod.m_H << " cost: " << prod.m_Cost << endl;
            }
            */

            listOfPriceList.insert(make_pair(materialID, priceList));
        } else {

            for (const auto & newProd : priceList->m_List) {

                bool found = false;

                for (auto & oldProd : listOfPriceList.at(materialID)->m_List) {

                    if ((newProd.m_H == oldProd.m_H && newProd.m_W == oldProd.m_W) ||
                        (newProd.m_H == oldProd.m_W && newProd.m_W == oldProd.m_H)) {

                        // We found the same product!
                        //printf("We found the same product!\n");
                        //cout << "Old W: " << oldProd.m_W << " Old H:" << oldProd.m_H << endl;
                        //cout << "New W: " << newProd.m_W << " New H:" << newProd.m_H << endl;

                        // Update its price!
                        if (newProd.m_Cost < oldProd.m_Cost) {
                            //cout << "Old cost: " << oldProd.m_Cost << " price: " << newProd.m_Cost << endl;
                            oldProd.m_Cost = newProd.m_Cost;
                        }

                        found = true;

                        break;
                    }
                }

                if (!found) {
                    //cout << "We are adding a new product into list: " << endl;
                    //cout << "H: " << newProd.m_H << " W:" << newProd.m_W << " Price: " << newProd.m_Cost << endl;
                    listOfPriceList.at(materialID)->Add(newProd);
                }
            }
        }
    }

    bool hasPriceListForEveryProducer(vector<AProducer> & producers, unsigned int materialID) const {
        bool hasPriceListForEveryProducer = true;

        for (const auto & producer : producers) {
            if (listOfProducers.find(materialID) == listOfProducers.end() || listOfProducers.at(materialID).find(producer) == listOfProducers.at(materialID).end()) {
                hasPriceListForEveryProducer = false;
                break;
            }
        }

        return hasPriceListForEveryProducer;
    }

    APriceList getPriceListForGivenMaterial(unsigned int materialID) const {
        return listOfPriceList.at(materialID);
    }

private:
    unordered_map<unsigned int, unordered_set<AProducer>> listOfProducers;
    unordered_map<unsigned int, APriceList> listOfPriceList;
};

class CWeldingCompany {
public:
    static void SeqSolve (APriceList priceList, COrder & order) {
        vector<COrder> orders;
        orders.push_back(order);

        ProgtestSolver(orders, priceList);

        order.m_Cost = orders[0].m_Cost;
    }

    void AddProducer (AProducer prod) {
        // Add producer into vector
        producers.push_back(prod);
    }

    void AddCustomer (ACustomer cust) {
        // Add customer into vector
        customers.push_back(cust);
    }

    void AddPriceList (AProducer prod, APriceList newPriceList) {
        // Lock the critical section
        unique_lock<mutex> lock (priceListMutex);

        // Add products in price list
        priceList.AddProductsToPriceList(prod, newPriceList);

        // Notify all threads, that price list has been updated
        updatedPriceList.notify_all();

        // Unlock the critical section
        lock.unlock();
    }

    // Start up helper and worker threads
    void Start (unsigned thrCount) {

        // Start up helper threads
        for (unsigned int i = 0; i < customers.size(); i++) {
            helperThreads.push_back(thread(&CWeldingCompany::Helper, this, i, customers[i]));
        }

        // Start up worker threads
        for (unsigned int i = 0; i < thrCount; i++) {
            workerThreads.push_back(thread(&CWeldingCompany::Worker, this, i));
        }
    }

    void Stop (void) {
        // Join helper threads
        for (unsigned int i = 0; i < helperThreads.size(); i++) {
            helperThreads[i].join();
        }

        // We would like to end all worker threads
        toEnd = true;

        // Notify all waiting workers
        bufferFree.notify_all();

        // Join worker threads
        for (unsigned int i = 0; i < workerThreads.size(); i++) {
            workerThreads[i].join();
        }
    }

    void Helper (unsigned int threadID, ACustomer customer) {
        //cout << "creating helper thread " << threadID << endl;

        AOrderList order = customer->WaitForDemand();

        // Retrieve orders
        while (order.get() != nullptr) {
            //cout << "We've got a new order!" << endl;

            // Lock the critical section
            unique_lock<mutex> bufferMtx (bufferMutex);

            // Wait until we can push into our jobs buffer
            bufferFull.wait(bufferMtx, [&] () {
                return buffer.size() < BUFFER_SIZE;
            });

            // Make pair (job) - with customer and order
            pair<ACustomer, AOrderList> newJob = make_pair(customer, order);

            // Add one job into our buffer
            buffer.push(newJob);

            // Notify one worker thread, that we've got something in our buffer
            bufferFree.notify_one();

            // Unlock the critical section
            bufferMtx.unlock();

            order = customer->WaitForDemand();
        }
    }

    void Worker (unsigned int threadID) {
        //cout << "creating worker thread " << threadID << endl;

        while (true) {
            // Lock the critical section
            unique_lock<mutex> bufferMtx (bufferMutex);

            // Buffer could have been free'd in the meantime,
            // wait until there's a job to process or until we need to finish
            bufferFree.wait(bufferMtx, [&] () {
                return buffer.size() > 0 || toEnd;
            });

            // When there's nothing in the buffer and we are about to finish,
            // break out of the loop
            if (buffer.size() == 0 && toEnd) {
                bufferMtx.unlock();
                break;
            }

            // Retrieve job from queue
            pair<ACustomer, AOrderList> job = buffer.front();

            // Remove the retrieved job from queue
            buffer.pop();

            // Notify one thread, that we've popped one job from buffer
            bufferFull.notify_one();

            // Lock out of the critical section
            bufferMtx.unlock();

            // If we do not have price list for some producer, we need to ask for it
            if (!priceList.hasPriceListForEveryProducer(producers, job.second->m_MaterialID)) {
                for (const auto & producer : producers) {
                    producer->SendPriceList(job.second->m_MaterialID);
                }
            }

            // Lock the critical section
            unique_lock<mutex> priceLock (priceListMutex);

            // Wait until we've got price for every producer
            updatedPriceList.wait(priceLock, [&] () {
                return priceList.hasPriceListForEveryProducer(producers, job.second->m_MaterialID);
            });


            // Lock out of the critical section
            priceLock.unlock();

            // Solve the problem
            ProgtestSolver(job.second->m_List, priceList.getPriceListForGivenMaterial(job.second->m_MaterialID));

            // Complete the job
            job.first->Completed(job.second);
        }
    }

private:
    const unsigned int BUFFER_SIZE = 15;

    vector<AProducer> producers;
    vector<ACustomer> customers;

    vector<thread> workerThreads;
    vector<thread> helperThreads;

    mutex bufferMutex;
    mutex priceListMutex;

    condition_variable bufferFull;
    condition_variable bufferFree;

    condition_variable updatedPriceList;

    queue<pair<ACustomer, AOrderList>> buffer;

    PriceList priceList;

    bool toEnd = false;
};

//-------------------------------------------------------------------------------------------------
#ifndef __PROGTEST__

int main(void) {
    using namespace std::placeholders;
    CWeldingCompany  test;

    AProducer p1 = make_shared<CProducerSync> ( bind ( &CWeldingCompany::AddPriceList, &test, _1, _2 ) );
    AProducerAsync p2 = make_shared<CProducerAsync> ( bind ( &CWeldingCompany::AddPriceList, &test, _1, _2 ) );
    test . AddProducer ( p1 );
    test . AddProducer ( p2 );
    test . AddCustomer ( make_shared<CCustomerTest> ( 2 ) );
    p2 -> Start ();
    test . Start ( 3 );
    test . Stop ();
    p2 -> Stop ();
    return 0;
}

#endif /* __PROGTEST__ */