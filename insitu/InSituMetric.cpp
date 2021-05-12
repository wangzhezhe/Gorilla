#include "InSituMetric.hpp"


MetricsSet InSituMetric::naiveGet()
{
  MetricsSet mset;
  mset.T = this->m_metricManager.getLastNmetrics("T", 1)[0];
  mset.At = this->m_metricManager.getLastNmetrics("At", 1)[0];
  mset.S = this->m_metricManager.getLastNmetrics("S", 1)[0];
  mset.Al = this->m_metricManager.getLastNmetrics("Al", 1)[0];
  mset.W = this->m_metricManager.getLastNmetrics("W", 1)[0];
  return mset;
}

MetricsSet InSituMetric::estimationGet(std::string lastDecision, int currStep, double burden)
{
  // total step can be acquired from the in-situ code
  MetricsSet mset = this->naiveGet();
  // let the initial value equals with each other and then update particular one
  MetricsSet emset = mset;
  double p = 0;
  double Tmin;

  if (lastDecision == "tightly")
  {
    // loosely coupled case is outdated
    // if (mset.Al < mset.At)
    // todo add the burdern not large
    if (mset.Al < mset.At)
    {
      emset.Al = mset.At;
      // insert the loosely coupled value
      // is not helpful
      // this->m_metricManager.putMetric("Al", emset.Al);
    }
    if (mset.Al > mset.At && burden < 0.2)
    {
      emset.Al = mset.At;
    }
    p = mset.At;
    // one asusmption is that the data size are same between different ranks
    // this->m_Tmin = std::min(this->m_Tmin, mset.T);
    // emset.T = m_Tmin;
  }
  else if (lastDecision == "loosely")
  {
    // tightly coupled values are outdated
    // if the al is 0, the staging task did not finish and we should still use the old values
    if (mset.Al < mset.At && mset.Al != 0)
    {
      emset.At = mset.Al;
      // insert the tightly coupled value
      // this->m_metricManager.putMetric("At", emset.At);
    }
    if (mset.Al > mset.At && burden < 0.2)
    {
      emset.At = mset.Al;
    }
    p = mset.T;
  }
  else
  {
    throw std::runtime_error("lastDecision is invalid");
  }
  this->m_savg = this->m_savg + 1.0 * (mset.S - this->m_savg) / (1.0 * currStep);
  this->m_pavg = this->m_pavg + 1.0 * (p - this->m_pavg) / (1.0 * currStep);
  double esim = (this->m_totalStep - 1 - currStep) * (this->m_savg + this->m_pavg);
  emset.S = esim;

  return emset;
}

void InSituMetric::adjustment(int totalProcs, int step, MetricsSet& mset, bool& ifTCAna,
  bool& ifWriteToStage, std::string& lastDecision, double burden, bool lastStep)
{

  int localTostage = 0;
  int totalTostage = 0;
  if (ifWriteToStage)
  {
    localTostage = 1;
  }
  MPI_Allreduce(&localTostage, &totalTostage, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

  // std::cout << "debug step " << step << " totalTostage " << totalTostage << " totalProcs "
  //          << totalProcs << std::endl;
  // this threshould can be parameter of the algorithm
  // if burden is ok

  // get subcomm for ana task
  int color = 0;
  MPI_Comm tightlyana_comm;
  int tightlyana_procs = 0;
  if (ifTCAna)
  {
    color = 1;
  }
  MPI_Comm_split(MPI_COMM_WORLD, color, this->m_rank, &tightlyana_comm);
  // most of them goes to tightly
  double totalAt = 0;
  double totalAl = 0;

  MPI_Allreduce(&mset.At, &totalAt, 1, MPI_DOUBLE, MPI_SUM, tightlyana_comm);
  MPI_Allreduce(&mset.Al, &totalAl, 1, MPI_DOUBLE, MPI_SUM, tightlyana_comm);
  MPI_Comm_size(tightlyana_comm, &tightlyana_procs);
  double avgAt = totalAt / (1.0 * tightlyana_procs);
  double avgAl = totalAl / (1.0 * tightlyana_procs);

  // make sure every process execute the split part
  // not the last step (currently, the last step is 39)
  // TODO remove the hardcode values here
  if (burden > 0.8 || lastStep)
  {
    return;
  }

  // some at may still harmfule here
  // even if we execute at for all

  if (ifTCAna)
  {
    if (mset.Al > 1.25 * avgAl || mset.At > 1.25 * avgAt)
    {
      ifWriteToStage = true;
      ifTCAna = false;
    }
  }

  if (totalTostage > 0 && totalTostage < totalProcs)
  {
    // there is inconsistency
    std::cout << "debug step " << step << " ifTCAna " << ifTCAna << " mset.At " << mset.At
              << " mset.T " << mset.T << " mset.Al " << mset.Al << " mset.S " << mset.S
              << std::endl;

    if (burden < 0.2 || burden > 0.8 || lastStep)
    {
      return;
    }

    if (ifTCAna == true)
    {
      // this part is harmful, more cases goes to loosely if we add this
      /* the al can still be large which cause the extra overhead, no matter last is tightly or
      loosely we use the huristic
      // to much differences between the An time
      if (lastDecision == "tightly")
      {
        // At is accurate
        double overhead = mset.T + mset.W + mset.Al - (mset.At + mset.S);
        double benifit = mset.At - mset.T;
        // std::cout << "debug step " << step << " mset.At " << mset.At << " mset.T " << mset.T
        //          << " benifit " << benifit << " overhead " << overhead << std::endl;
        if (benifit>0 && benifit > overhead)
        {
          // adjust the decision
          ifWriteToStage = true;
          ifTCAna = false;
        }
      }
      */

      // if most of them decide to go staging, server is ok
      // some of them decide to go tightly, this might caused by outdated of At, false assume it is
      // too small
      // we might not know if it is caused dy the server overload or the data is outdated
      // switch this part may decrease the harmful value and not give too much burdern to the
      // staging
      // if (lastDecision == "loosely" && totalTostage > totalProcs / 2)
      // if (lastDecision == "loosely")
      //{
      // not try at, since it is possible caused by the outdated at
      // the al data is accurate
      if (mset.Al > mset.T || mset.At > mset.T)
      {
        // this may caused by the false anticipation of too small At
        ifWriteToStage = true;
        ifTCAna = false;
      }
    }
  }
}

void InSituMetric::decideTaskPlacement(int step, int rank, int totalprocs, double burdern,
  std::string strategy, bool& ifTCAna, bool& ifWriteToStage, bool lastStep)
{

  if (step <= 1)
  {
    ifTCAna = true;
    ifWriteToStage = false;

    return;
  }
  if (step == 2)
  {
    ifTCAna = false;
    ifWriteToStage = true;

    return;
  }

  // get the last situation
  std::string lastDecision;
  if (ifTCAna == true && ifWriteToStage == false)
  {
    lastDecision = "tightly";
  }
  else if (ifTCAna == false && ifWriteToStage == true)
  {
    lastDecision = "loosely";
  }
  else
  {
    throw std::runtime_error("last decisions are in valid");
  }

  // these two variables need to be restet every time
  ifTCAna = false;
  ifWriteToStage = false;
  double currentsavedTime = 0;

  MetricsSet mset;
  if (strategy == "dynamicNaive")
  {
    mset = this->naiveGet();
  }
  else if (strategy.find("dynamicEstimation") != std::string::npos)
  {
    // we assume the freq is 1 and currSimStep equals to the currInSituStep
    mset = this->estimationGet(lastDecision, step, burdern);
  }
  else
  {
    throw std::runtime_error("unsupprted strategy");
  }

  if (mset.S >= (mset.W + mset.Al))
  {
    if (mset.At >= mset.T)
    {
      ifWriteToStage = true;
    }
    else
    {
      ifTCAna = true;
    }
    // currentsavedTime = abs(At - T);
  }
  else
  {
    if (mset.At + mset.S >= (mset.T + mset.W + mset.Al))
    {
      ifWriteToStage = true;
    }
    else
    {
      ifTCAna = true;
    }
    // currentsavedTime = abs((At + S) - (T + W + Al));
  }

  // totalsavedTime = totalsavedTime + currentsavedTime;
  // std::string metricName = "Saved";
  // gsinsitu.m_metricManager.putMetric(metricName, currentsavedTime);

  bool oldifTCAna = ifTCAna;
  bool oldifWriteToStage = ifWriteToStage;
  if (strategy.find("Adjust") != std::string::npos && step >= 3)
  {
    adjustment(totalprocs, step, mset, ifTCAna, ifWriteToStage, lastDecision, burdern, lastStep);
  }
  if (ifTCAna != oldifTCAna || ifWriteToStage != oldifWriteToStage)
  {
    std::cout << "after adjustment rank " << rank << " step " << step << " ifTCAna " << ifTCAna
              << " ifWriteToStage " << ifWriteToStage << std::endl;
  }
}