#include "../../testing/testing.hpp"
#include "../indexer_tool/feature_bucketer.hpp"
#include "../feature.hpp"
#include "../../base/stl_add.hpp"

namespace
{
  class PushBackFeatureDebugStringOutput
  {
  public:
    typedef map<string, vector<string> > * InitDataType;

    PushBackFeatureDebugStringOutput(string const & name, InitDataType const & initData)
      : m_pContainer(&((*initData)[name])) {}

    void operator() (Feature const & feature)
    {
      m_pContainer->push_back(feature.DebugString());
    }

  private:
    vector<string> * m_pContainer;
  };

  typedef feature::CellFeatureBucketer<
      PushBackFeatureDebugStringOutput,
      feature::SimpleFeatureClipper,
      MercatorBounds,
      RectId
  > FeatureBucketer;

  Feature MakeFeature(FeatureBuilder const & fb)
  {
    vector<char> data;
    fb.Serialize(data);
    return Feature(data, 0);
  }
}

UNIT_TEST(FeatureBucketerSmokeTest)
{
  map<string, vector<string> > out, expectedOut;
  FeatureBucketer bucketer(1, &out);

  FeatureBuilder fb;
  fb.AddPoint(m2::PointD(10, 10));
  fb.AddPoint(m2::PointD(20, 20));
  bucketer(MakeFeature(fb));

  expectedOut["3"].push_back(MakeFeature(fb).DebugString());
  TEST_EQUAL(out, expectedOut, ());

  vector<string> bucketNames;
  bucketer.GetBucketNames(MakeBackInsertFunctor(bucketNames));
  TEST_EQUAL(bucketNames, vector<string>(1, "3"), ());
}
