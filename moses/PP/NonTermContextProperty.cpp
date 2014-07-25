#include <string>
#include <assert.h>
#include "moses/PP/NonTermContextProperty.h"
#include "moses/Util.h"
#include "moses/FactorCollection.h"
#include "util/exception.hh"

using namespace std;

namespace Moses
{
NonTermContextProperty::ProbStore::ProbStore()
:m_vecInd(4)
,m_vecJoint(2)
,m_totalCount(0)
{}

void NonTermContextProperty::ProbStore::AddToMap(size_t index, const Factor *factor, float count)
{
	Map &map = m_vecInd[index];

	Map::iterator iter = map.find(factor);
	if (iter == map.end()) {
		map[factor] = count;
	}
	else {
		float &currCount = iter->second;
		currCount += count;
	}

	m_totalCount += count;
}

void NonTermContextProperty::ProbStore::AddToMap(size_t index, const Factor *left, const Factor *right, float count)
{
  UTIL_THROW_IF2(index >= m_vecJoint.size(), "index (" << index << ") is bigger than vector (" << m_vecJoint.size() << ")");

	MapJoint &map = m_vecJoint[index];

	std::pair<const Factor*, const Factor*> key(left, right);
	MapJoint::iterator iter = map.find(key);
	if (iter == map.end()) {
		map[key] = count;
	}
	else {
		float &currCount = iter->second;
		currCount += count;
	}

}

float NonTermContextProperty::ProbStore::GetProb(size_t contextInd,
			const Factor *factor,
			float smoothConstant) const
{
  float count = GetCount(contextInd, factor, smoothConstant);
  float total = GetTotalCount(contextInd, smoothConstant);
  float ret = count / total;
  return ret;
}

float NonTermContextProperty::ProbStore::GetProb(size_t contextInd,
			const Factor *left,
			const Factor *right,
			float smoothConstant) const
{
  float count = GetCount(contextInd, left, right, smoothConstant);
  float total = GetTotalCount(contextInd, smoothConstant);
  float ret = count / total;
  return ret;
}

float NonTermContextProperty::ProbStore::GetCount(size_t contextInd,
			const Factor *factor,
			float smoothConstant) const
{
	const Map &map = m_vecInd[contextInd];

	float count = smoothConstant;
	Map::const_iterator iter = map.find(factor);
	if (iter == map.end()) {
		// nothing
	}
	else {
		count += iter->second;
	}

	return count;
}

float NonTermContextProperty::ProbStore::GetCount(size_t contextInd,
			const Factor *left,
			const Factor *right,
			float smoothConstant) const
{
	const MapJoint &map = m_vecJoint[contextInd];

	float count = smoothConstant;
	std::pair<const Factor*, const Factor*> key(left, right);
	MapJoint::const_iterator iter = map.find(key);
	if (iter == map.end()) {
		// nothing
	}
	else {
		count += iter->second;
	}

	return count;
}

float NonTermContextProperty::ProbStore::GetTotalCount(size_t contextInd, float smoothConstant) const
{
	const Map &map = m_vecInd[contextInd];
	return m_totalCount + smoothConstant * map.size();
}

///////////////////////////////////////

NonTermContextProperty::NonTermContextProperty()
{
}

NonTermContextProperty::~NonTermContextProperty()
{
	//RemoveAllInColl(m_probStores);
}

void NonTermContextProperty::ProcessValue(const std::string &value)
{
  /*
   format is:
	 {{NonTermContext 1 0 ! dfc ! !  0.181818 0 ! 警告 ! !  0.181818}}

	1. number of NTs
	2. NT index (from 0)
	3. left outer
	4. left inner
	5. right inner
	6. right outer
	7. count
	repeat from (2)
  */

  vector<string> toks;
  Tokenize(toks, value);

  FactorCollection &fc = FactorCollection::Instance();

  size_t numNT = Scan<size_t>(toks[0]);
  m_probStores.resize(numNT);

  // indie
  size_t ind = 1;
  while (ind < toks.size()) {
	  vector<const Factor *> factorsInd;

	  // read in 1 block of all NT in the rule
	  for (size_t nt = 0; nt < numNT; ++nt) {
		  size_t ntInd = Scan<size_t>(toks[ind]);
		  assert(nt == ntInd);
		  ++ind;

		  for (size_t contextInd = 0; contextInd < 4; ++contextInd) {
			//cerr << "toks[" << ind << "]=" << toks[ind] << endl;
  			const Factor *factor = fc.AddFactor(toks[ind], false);
  			factorsInd.push_back(factor);
		    ++ind;
		  }
	  }

	  // done with the context. Just get the count and put it all into data structures
	  // cerr << "count=" << toks[ind] << endl;
	  float count = Scan<float>(toks[ind]);
	  ++ind;

	  // indie
	  for (size_t i = 0; i < factorsInd.size(); ++i) {
		  size_t ntInd = i / 4;
		  size_t contextInd = i % 4;
		  const Factor *factor = factorsInd[i];
		  AddToMap(ntInd, contextInd, factor, count);
	  }

	  // joint
	  for (size_t i = 0; i < factorsInd.size(); i += 4) {
		  size_t ntInd = i / 4;
		  const Factor *outsideLeft = factorsInd[i];
		  const Factor *insideLeft = factorsInd[i + 1];
		  const Factor *insideRight = factorsInd[i + 2];
		  const Factor *outsideRight = factorsInd[i + 3];
  		//cerr << *outsideLeft << " " << *insideLeft << " " << *insideRight << " " << *outsideRight << endl;

		  AddToMap(ntInd, 0, insideLeft, insideRight, count);
		  AddToMap(ntInd, 1, outsideLeft, outsideRight, count);
	  }
  } // while (ind < toks.size()) {

}

void NonTermContextProperty::AddToMap(size_t ntIndex, size_t contextIndex, const Factor *factor, float count)
{
  if (ntIndex <= m_probStores.size()) {
	  m_probStores.resize(ntIndex + 1);
  }

  ProbStore &probStore = m_probStores[ntIndex];
  probStore.AddToMap(contextIndex, factor, count);
}

void NonTermContextProperty::AddToMap(size_t ntIndex,
									size_t contextIndex,
									const Factor *left,
									const Factor *right,
									float count)
{
    //cerr << "ntIndex=" << ntIndex << " m_probStores=" << m_probStores.size() << endl;

    UTIL_THROW_IF2(ntIndex >= m_probStores.size(), "NT index (" << ntIndex << ") is bigger than vector (" << m_probStores.size() << ")");
	  ProbStore &probStore = m_probStores[ntIndex];
	  probStore.AddToMap(contextIndex, left, right, count);
}

float NonTermContextProperty::GetProb(size_t ntInd,
			size_t contextInd,
			const Factor *factor,
			float smoothConstant) const
{
	UTIL_THROW_IF2(ntInd >= m_probStores.size(), "Invalid nt index=" << ntInd);
	const ProbStore &probStore = m_probStores[ntInd];
	float ret = probStore.GetProb(contextInd, factor, smoothConstant);
	return ret;
}

float NonTermContextProperty::GetProb(size_t ntInd,
		  size_t contextInd,
		  const Factor *left,
		  const Factor *right,
		  float smoothConstant) const
{
	UTIL_THROW_IF2(ntInd >= m_probStores.size(), "Invalid nt index=" << ntInd);
	const ProbStore &probStore = m_probStores[ntInd];
	float ret = probStore.GetProb(contextInd, left, right, smoothConstant);
	return ret;
}



} // namespace Moses

