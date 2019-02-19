#include <cppunit/extensions/HelperMacros.h>

#include "DataFormats/Math/interface/GeantUnits.h"

#include "cppunit/TestAssert.h"
#include "cppunit/TestFixture.h"

using namespace geant_units;
using namespace geant_units::operators;

class testDDUnits : public CppUnit::TestFixture {
  
  CPPUNIT_TEST_SUITE( testDDUnits );
  CPPUNIT_TEST( checkUnits );
  CPPUNIT_TEST_SUITE_END();

public:

  void setUp() override{}
  void tearDown() override {}
  void checkUnits();
};

CPPUNIT_TEST_SUITE_REGISTRATION(testDDUnits);

void testDDUnits::checkUnits()
{
  std::cout << "\nMy pi: " << _pi
	    << " == " << 1_pi
	    << " == " << 1._pi << "\n";
  
  CPPUNIT_ASSERT( M_PI == _pi );
  CPPUNIT_ASSERT( M_PI == 1_pi );
  CPPUNIT_ASSERT( M_PI == 1._pi );

  double twoPiAngle = 2_pi;
  std::cout << "My 2pi angle: " << twoPiAngle
	    << " [rad] == " << CMS_CONVERT_TO( twoPiAngle, deg )
	    << " [deg]\n";

  CPPUNIT_ASSERT( 2*M_PI == 2*_pi );
  CPPUNIT_ASSERT( 2*M_PI == twoPiAngle );
  CPPUNIT_ASSERT( 2*M_PI == 2_pi );
  CPPUNIT_ASSERT( 2*M_PI == 2._pi );
  
  CPPUNIT_ASSERT( 90_deg == _pi/2 );
  CPPUNIT_ASSERT( 120_deg == 2_pi/3 );
  CPPUNIT_ASSERT( 135_deg == 3_pi/4 );
  CPPUNIT_ASSERT( 150_deg == 5_pi/6 );

  double angle90 = CMS_CONVERT_TO( 1_pi/2, deg );
  CPPUNIT_ASSERT( angle90 == 90 );

  double angle120 = CMS_CONVERT_TO( 2_pi/3, deg );
  CPPUNIT_ASSERT( angle120 == 120. );

  double angle135 = CMS_CONVERT_TO( 3_pi/4, deg );
  CPPUNIT_ASSERT( angle135 == 135. );

  double angle150 = CMS_CONVERT_TO( 5_pi/6, deg );
  CPPUNIT_ASSERT( angle150 == 150. );
}
