/*
 * Trace3.cpp
 *
 *  Created on: Dec 19, 2012
 *      Author: carolinux
 */

/*
 * Experiment3.cpp
 *
 *  Created on: Oct 8, 2012
 *      Author: carolinux
 */

/*
 * Experiment1.cpp
 *
 *  Created on: Oct 4, 2012
 *      Author: carolinux
 */

#include <iostream>
using namespace std;
#include<string>
#include "Util.h"
#include "Database.h"
#include "Query.h"
#include "Synopsis.h"
#include "Bloom.h"
#include <assert.h>
#include <fstream> //for parsing config file
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include "MultiTrie.h"
#include "Synopsis/QuadSynopsis.h"
#include "Synopsis/SynopsisMD2.h"
#include "Synopsis/SynopsisMD.h"
#include "Synopsis/Statistics.h"
#include <sstream>
#include <sched.h>
#include "curves/Curve.h"

#include "Database/DataqueryViz.h"
#define CFG_PARAMS 24
//EXP MODE//
bool VARYINT = false; // vary interval size
bool VARYFS = false; //vary filter size
bool VARYDBS = false;
bool VARYPART = false;
bool SKIPBL = false;
bool ZIPFUPDATES = false;

//  Database Parameters
uint DBDOMAIN;  // domain of the keys of the database
uint DBSIZE = 1000;    // number of keys in the cold store of the database
bool ZIPFDATA = 0 ;
double ZIPFDATAFACTOR = 1.2;  // zipf factor for database

int MAX_FSIZE = 10000;

int EXPQUERIES = 10000;


int TRAININGQUERIES = 5000;    // number of range queries used for training

double MEANT = 50;
double STDDEVT = 17;

int JUSTPOINTQUERIES = 0;
double MEAN=7;
double STDDEV=4;
bool USEOLDZIPFINTERVAL;

bool ZIPFQUERIES = 0;
double ZIPFQUERIESFACTOR = 1.2;     // zipf factor for distribution of length

//  Access Filter parameters

int RANGEFILTERSIZE = 1000;  // size of synopsis in nodes
//int BLOOMFILTERSIZE;//  2 * RANGEFILTERSIZE  // size of poor man's Bloom filter in bits


// Book-keeping
char * OUTPUTFILE;

int main(int argc, char* argv[]) {


	  bool useH = false;

	JUSTPOINTQUERIES = atoi(argv[1]);
	if(argc==7)
			useH = atoi(argv[6]);
	char data_corr = argv[2][0];
	char query_corr = argv[3][0];
	char * folder = argv[4];



  cout<<"Experiment 3 - correlated/anti-correlated experiments"<<endl;

  int dim = atoi(argv[5]);
  int MAX = 15;
  DBSIZE = 10;



  if(dim ==3)
  {
	  MAX = 127;
	  STDDEVT =5;
	  MEANT =10;
	  MEAN = 3;
	  STDDEV =2;
  }


    vector<int> domains(dim);

    vector<Curve::attribute> attr(dim);
    for(int i=0;i<dim;i++)
    {
    	DBDOMAIN = DBDOMAIN * MAX;
    	attr[i].lowerb = 0;
    	attr[i].higherb = MAX;
    	domains[i] = MAX;
    }
    DBDOMAIN--;


    vector<int> lowp(dim);
	for(int i=0;i<dim;i++)
	   lowp[i] = attr[i].lowerb;
	vector<int> highp(dim);
	for(int i=0;i<dim;i++)
		   lowp[i] = attr[i].higherb;

  Database db(attr,'c');
  db.populateMD(DBSIZE,domains,data_corr);
  Database dbz(&db,'z');

  Database dbh(&db,'h');


  //dbz.populateMD(DBSIZE,domains,data_corr);
  //dbh.populateMD(DBSIZE,domains,data_corr);

  db.plot();
  	  /* START OF EXP STUFF */

  cout<<"generating queries"<<endl;
  Query qGen(domains,EXPQUERIES,
         		  		  MEAN,STDDEV,query_corr);
  cout<<"generated queries"<<endl;

         /* start outputting stuffs */

         stringstream s0,s1,s2,s3,s4;

         s0<<"rm -r "<<folder;

         system(s0.str().c_str());
         system("sleep 5");

         s1<<"mkdir "<<folder;
         system(s1.str().c_str());


         s2<<"cp pre/gnuplot.txt "<<folder<<"/gnuplot.txt";


         system(s2.str().c_str());
         system("sleep 10");
         s2.clear();
         if(JUSTPOINTQUERIES)
         {
        	 s2<<"Point queries";
         }
         else
        	 s2<<"Range queries";

         s2<<data_corr<<" data, "<<query_corr<<" queries \n";



         s3<<folder<<"/results.txt";

         FILE * results = fopen(s3.str().c_str(),"w");
         fprintf(results,"SIZE \t\tQD \t\tKD \t\tKDA \t\tCC \t\tZC \t\tHC \t\tBL Correlated data \n");
         fprintf(results,"%s",s2.str().c_str());

         fflush(results);

         s4<<folder<<"/data_queries.bmp";

         DataqueryViz plotter(&db,&qGen,JUSTPOINTQUERIES);

         plotter.plot2D(s4.str());

         /* END OF EXP STUFF */



  QuadSynopsis quad(dim,&db,lowp,highp,32);
  SynopsisMD2 multi(dim,&db,lowp,highp,32);
  SynopsisMD2 multi_alt(dim,&db,lowp,highp,32);
  SynopsisMD sync(attr,'c',RANGEFILTERSIZE,&db);
  SynopsisMD synz(attr,'z',RANGEFILTERSIZE,&dbz);
  SynopsisMD synh(attr,'h',RANGEFILTERSIZE,&dbh);
  multi_alt.setAlternate();



  cout<<"learn from data distribution"<<endl;


  quad.perfect(&db);
  quad.takeSnapshot("quad");




  multi.perfect(&db);
  multi_alt.perfect(&db);
  sync.perfect();
  synz.perfect();
  synh.perfect();


  //vector<Query::QueryMD_t> q(4);

  return 0;





  cout<<"started training"<<endl;


  Query qTrain1(domains,TRAININGQUERIES/3,
	  		  MEAN,STDDEV,query_corr);
    for (int i=0; i<TRAININGQUERIES/3; i++) {
      Query::QueryMD_t q = qTrain1.nextQueryMD();

      //if(JUSTPOINTQUERIES)
    //	  q.low = q.high;
      bool qR = db.rangeQuery(q.low,q.high);

      if(i%100 == 0)
    	  cout<<i<<" th query (1st training)"<<endl;
      quad.handleQuery(q,true,qR);
      multi.handleQuery(q,true,qR);

      multi_alt.handleQuery(q,true,qR);

      /* space filling */
      sync.handleQuery(q.low,q.high,true);

      synz.handleQuery(q.low,q.high,true);
      return 0;
      if(useH)
      	synh.handleQuery(q.low,q.high,true);





    }

    Query qTrain2(domains,2*TRAININGQUERIES/3,
	  		  MEANT,STDDEVT,query_corr);
        for (int i=0; i<2*TRAININGQUERIES/3; i++) {
          Query::QueryMD_t q = qTrain2.nextQueryMD();

          if(i%100 == 0)
            	  cout<<i<<" th query (2nd training)"<<endl;

          bool qR = db.rangeQuery(q.low,q.high);
          quad.handleQuery(q,true,qR);
          multi.handleQuery(q,true,qR);
         multi_alt.handleQuery(q,true,qR);

         /* space filling */
         sync.handleQuery(q.low,q.high,true);
         synz.handleQuery(q.low,q.high,true);
	 if(useH)
         	synh.handleQuery(q.low,q.high,true);






        }

        quad.set(0);
        multi.set(0);
        multi_alt.set(0);

        sync.set(0);
        synh.set(0);
        synz.set(0);

        /*START LOOP */

        for(RANGEFILTERSIZE=MAX_FSIZE;RANGEFILTERSIZE>=1000;RANGEFILTERSIZE-=1000)
        {

        	Bloom bl(db.curve->getDomain(), DBSIZE, &db,RANGEFILTERSIZE,db.curve);
        qGen.reset();

        quad.truncate(RANGEFILTERSIZE);
        multi.truncate(RANGEFILTERSIZE);

        sync.truncate(RANGEFILTERSIZE);
        synz.truncate(RANGEFILTERSIZE);
        synh.truncate(RANGEFILTERSIZE);
        multi_alt.truncate(RANGEFILTERSIZE);

        quad.convert(0);
        multi.convert(0);
        multi_alt.convert(0);
        sync.convert(0);
		synh.convert(0);
		synz.convert(0);


 		quad.stats.reset();
 		multi_alt.stats.reset();
 		multi.stats.reset();
 		synh.stats.reset();
 		sync.stats.reset();
 		synz.stats.reset();
 		bl.stats.reset();


		int tp = 0;
		int tn = 0;

		printf("SIZE \t\tQD \t\tKD \t\tKDA \t\tCC \t\tZC \t\tHC \t\tBL \n");


		 for (int i=0; i<EXPQUERIES; i++)
		   {

				Query::QueryMD_t q = qGen.nextQueryMD();

				  if(i%100 == 0)
				    	  cout<<i<<" th query (benchmark)"<<endl;

				if(JUSTPOINTQUERIES)
					q.low=q.high;

				//cout<<"Pt:"<<q.right<<endl;

				bool qR = db.rangeQuery(q.low,q.high);

				if(qR)
				  tp++;
				else
				  tn++;

				 /* explicit multi dimensional trees*/

				 quad.handleQuery(q,false,qR); /* quad tree */
				 multi.handleQuery(q,false,qR); /* k-d tree with specified split dimension */
				 multi_alt.handleQuery(q,false,qR); /* k-d tree with alternating
				  	  	  	  	  	  	  	  	  	  	split dimension */

				 /* space filling curves */
				 sync.handleQuery(q,false); /* composite key */
				 synz.handleQuery(q,false); /* Z-order (bit interleaving) */
				 if(useH)
					synh.handleQuery(q,false); /* Hilbert curve */

				 /* Bloom filter*/

				 bl.handleQuery(q,qR);


				 if(1==0 && i %100 == 0)
				 {
					 printf("%d \t%f \t%f \t%f \t%f \t%f \t%f \t%f  \n",
												RANGEFILTERSIZE,
												quad.stats.getFpr(),
												multi.stats.getFpr(),
												multi_alt.stats.getFpr(),
												sync.stats.getFpr(),
												synz.stats.getFpr(),
												synh.stats.getFpr(),
												bl.stats.getFpr());
				 }

		   }




		  printf("%d \t%f \t%f \t%f \t%f \t%f \t%f \t%f  \n",
		  	    			RANGEFILTERSIZE,
		  	    			quad.stats.getFpr(),
		  	    			multi.stats.getFpr(),
		  	    			multi_alt.stats.getFpr(),
		  	    			sync.stats.getFpr(),
		  	    			synz.stats.getFpr(),
		  	    			synh.stats.getFpr(),
		  	    			bl.stats.getFpr());

		  fprintf(results,"%d \t%f \t%f \t%f \t%f \t%f \t%f \t%f  \n",
		  	    			RANGEFILTERSIZE,
		  	    			quad.stats.getFpr(),
		  	    			multi.stats.getFpr(),
		  	    			multi_alt.stats.getFpr(),
		  	    			sync.stats.getFpr(),
		  	    			synz.stats.getFpr(),
		  	    			synh.stats.getFpr(),
		  	    			bl.stats.getFpr());
		  fflush(results);

		  sync.stats.print();
		  synz.stats.print();
		  synh.stats.print();
		  getchar();



        }



        fclose(results);
/*
        stringstream cmd;
        cmd<<"cd "<<folder;
        system(cmd.str().c_str());

        system("gnuplot gnuplot.txt");
        system("cd ../");
*/



 return 0;
}




