#include "testClass.h"


namespace couchit {


	void runMiniHttpTests(TestSimple &tst) ;
	void testUUIDs(TestSimple &tst) ;
	void runTestBasics(TestSimple &tst);
	void runTestLocalview(TestSimple &tst);


}

int main(int argc, char *argv[]) {
	TestSimple tst;

	couchit::runTestLocalview(tst);
	couchit::runMiniHttpTests(tst);
	couchit::testUUIDs(tst);
	couchit::runTestBasics(tst);


	return tst.didFail()?1:0;

}


#if 0

namespace couchit {

	void runQueryServer();

	using namespace LightSpeed;


	class Main : public App {
	public:
		
		virtual integer start(const Args &args);

	};

	LightSpeed::integer Main::start(const Args &args)
	{
		ConsoleA console;
		integer retval = 0;
		TestCollector &collector = Singleton<TestCollector>::getInstance();
		if (args.length() < 2) {
			SeqFileOutBuff<> out(StdOutput().getStream());
			collector.runTests(ConstStrW(),out);
		}
		else for (std::size_t i = 1; i < args.length(); i++) {
			ConstStrW itm = args[i];
			if (itm == L"list") {
				StringA lst = collector.listTests();
				console.print("%1") << lst;
				console.print("\nQueryServer backend: queryserver");

			} else if (itm == L"queryserver") {
				runQueryServer();
			}
			else {
				SeqFileOutBuff<> out(StdOutput().getStream());
				if (collector.runTests(args[i], out )) retval = 1;
			}
		}
		return retval;
	}


	static Main theApp;
}

#endif
