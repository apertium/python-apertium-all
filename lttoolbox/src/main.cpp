#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>

namespace py = pybind11;

#include <lttoolbox/acx.h>
#include <lttoolbox/alphabet.h>
#include <lttoolbox/fst_processor.h>
#include <lttoolbox/string_utils.h>
#include <lttoolbox/transducer.h>

// class for backwards compatibility with old SWIG bindings
class FST: public FSTProcessor {
public:
	/**
	 * Imitates functionality of lt-proc using file path
	 */
	FST(char *dictionary_path) {
		FILE *dictionary = fopen(dictionary_path, "rb");
		load(dictionary);
		fclose(dictionary);
	}

	void lt_proc(int argc, char **argv,
				 const char *input_path, const char *output_path) {
		InputFile input;
		input.open(input_path);
		UFILE* output = u_fopen(output_path, "w", NULL, NULL);
		int cmd = 0;
		int c = 0;
		optind = 1;
		GenerationMode bilmode = gm_unknown;
		while (true) {
			c = getopt(argc, argv, "abcdeglmnoptwxzCIW");
			if (c == -1) {
				break;
			}
			switch(c) {
			case 'a':
			case 'b':
			case 'e':
			case 'g':
			case 'p':
			case 't':
			case 'x':
				if(cmd == 0) {
					cmd = c;
				} else if (cmd == 'g' && c == 'b') {
					cmd = c;
				}
				break;
			case 'c':
				setCaseSensitiveMode(true);
				break;
			case 'd':
				if (cmd == 0) cmd = 'g';
				bilmode = gm_all;
				break;
			case 'l':
				if (cmd == 0) cmd = 'g';
				bilmode = gm_tagged;
				break;
			case 'm':
				if (cmd == 0) cmd = 'g';
				bilmode = gm_tagged_nm;
				break;
			case 'n':
				if (cmd == 0) cmd = 'g';
				bilmode = gm_clean;
				break;
			case 'o':
				if (cmd == 0) cmd = 'b';
				setBiltransSurfaceForms(true);
				break;
			case 'w':
				setDictionaryCaseMode(true);
				break;
			case 'z':
				setNullFlush(true);
				break;
			case 'C':
				if (cmd == 0) cmd = 'g';
				bilmode = gm_carefulcase;
				break;
			case 'I':
				setUseDefaultIgnoredChars(false);
				break;
			case 'W':
				setDisplayWeightsMode(true);
				break;
			default:
				break;
			}
		}

		switch(cmd) {
		case 'b':
			initBiltrans();
			bilingual(input, output, bilmode);
			break;

		case 'e':
			initDecomposition();
			analysis(input, output);
			break;

		case 'g':
			initGeneration();
			generation(input, output, bilmode);
			break;

		case 'p':
			initPostgeneration();
			postgeneration(input, output);
			break;

		case 't':
			initPostgeneration();
			transliteration(input, output);
			break;

		case 'x':
			initPostgeneration();
			intergeneration(input, output);
			break;

		case 'a':
		default:
			initAnalysis();
			analysis(input, output);
			break;
		}

		u_fclose(output);
	}

	void lt_proc_vec(const std::vector<std::string>& args,
					 const std::string& input_path,
					 const std::string& output_path) {
		size_t argc = args.size();
		char** argv = new char*[argc+1];
		for (size_t i = 0; i < argc; i++) {
			argv[i] = new char[args[i].size()+1];
			for (size_t j = 0; j < args[i].size(); j++) {
				argv[i][j] = args[i][j];
			}
			argv[i][args[i].size()] = 0;
		}
		argv[argc] = 0;
		lt_proc(argc, argv,	input_path.c_str(), output_path.c_str());
	}
	void lt_proc_len_vec(int argc, // which we ignore
						 const std::vector<std::string>& args,
						 const std::string& input_path,
						 const std::string& output_path) {
		lt_proc_vec(args, input_path, output_path);
	}
};

PYBIND11_MODULE(lttoolbox, m) {
	m.doc() = "LTTOOLBOX!";

	{
		auto fst = m.def_submodule("fst", "blah");

		py::enum_<Alphabet::Side>(fst, "Side")
			.value("Left", Alphabet::Side::left)
			.value("Right", Alphabet::Side::right)
			.export_values();

		py::class_<Alphabet>(fst, "Alphabet")
			.def(py::init<>())
			.def("includeSymbol", &Alphabet::includeSymbol)
			.def("__call__",
				 [](Alphabet& a, UStringView s) {
					 return a(s);
				 })
			.def("__call__",
				 [](Alphabet& a, int32_t c1, int32_t c2) {
					 return a(c1, c2);
				 })
			.def("isSymbolDefined", &Alphabet::isSymbolDefined)
			.def("__len__", &Alphabet::size)
			.def("write", &Alphabet::write)
			.def("read", &Alphabet::read)
			//.def("writeSymbol", &Alphabet::writeSymbol) // TODO: need UFILE
			.def("getSymbol",
				 [](Alphabet& a, int32_t symbol) {
					 UString s;
					 a.getSymbol(s, symbol);
					 return s;
				 })
			.def("isTag", &Alphabet::isTag)
			.def("setSymbol", &Alphabet::setSymbol)
			.def("decode", &Alphabet::decode)
			.def("symbolsWhereLeftIs", &Alphabet::symbolsWhereLeftIs)
			.def("createLoopbackSymbols", &Alphabet::createLoopbackSymbols)
			.def("tokenize", &Alphabet::tokenize)
			.def("sameSymbol", &Alphabet::sameSymbol);

		py::class_<Transducer>(fst, "Transducer")
			.def(py::init<>())
			.def("weighted", &Transducer::weighted)
			.def("insertSingleTransduction",
				 &Transducer::insertSingleTransduction)
			.def("insertNewSingleTransduction",
				 &Transducer::insertNewSingleTransduction)
			.def("insertTransducer", &Transducer::insertTransducer)
			.def("linkStates", &Transducer::linkStates)
			.def("isFinal", &Transducer::isFinal)
			.def("recognise", &Transducer::recognise)
			.def("setFinal", &Transducer::setFinal)
			.def("getInitial", &Transducer::getInitial)
			.def("closure",
				 py::overload_cast<int, int>(&Transducer::closure, py::const_))
			.def("closure", py::overload_cast<int, const std::set<int>&>(&Transducer::closure, py::const_))
			.def("closure_all", &Transducer::closure_all)
			.def("joinFinals", &Transducer::joinFinals)
			.def("getFinals", &Transducer::getFinals)
			.def("getTransitions", &Transducer::getTransitions)
			.def("reverse", &Transducer::reverse)
			//.def("show", &Transducer::show) // TODO: need UFILE
			.def("determinize", &Transducer::determinize)
			.def("minimize", &Transducer::minimize)
			.def("optional", &Transducer::optional)
			.def("oneOrMore", &Transducer::oneOrMore)
			.def("zeroOrMore", &Transducer::zeroOrMore)
			.def("clear", &Transducer::clear)
			.def("isEmpty",
				 py::overload_cast<int>(&Transducer::isEmpty, py::const_))
			.def("isEmpty",
				 py::overload_cast<>(&Transducer::isEmpty, py::const_))
			.def("hasNoFinals", &Transducer::hasNoFinals)
			.def("__len__", &Transducer::size)
			.def("numberOfTransitions", &Transducer::numberOfTransitions)
			.def("getStateSize", &Transducer::getStateSize)
			.def("write", &Transducer::write)
			.def("read", &Transducer::read)
			.def("unionWith", &Transducer::unionWith)
			.def("appendDotStar", &Transducer::appendDotStar)
			.def("moveLemqsLast", &Transducer::moveLemqsLast)
			.def("copyWithTagsFirst", &Transducer::copyWithTagsFirst)
			.def("trim", &Transducer::trim)
			.def("compose", &Transducer::compose)
			.def("composeLabel", &Transducer::composeLabel)
			.def("updateAlphabet", &Transducer::updateAlphabet)
			.def("invert", &Transducer::invert)
			.def("deleteSymbols", &Transducer::deleteSymbols)
			.def("epsilonizeSymbols", &Transducer::epsilonizeSymbols)
			.def("applyACX", &Transducer::applyACX);

		fst.def("readACX", &readACX);
	}

	{
		// skip functions that are in python stdlib
		auto su = m.def_submodule("string_utils", "blah");
		su.def("split_escaped", &StringUtils::split_escaped, "blah");
		su.def("getcase", &StringUtils::getcase, "blah");
		su.def("copycase", &StringUtils::copycase, "blah");
		su.def("caseequal", &StringUtils::caseequal, "blah");
		su.def("merge_wblanks", &StringUtils::merge_wblanks, "blah");
	}

	py::class_<FST>(m, "FST")
		.def(py::init<char*>())
		.def("lt_proc", &FST::lt_proc_vec)
		.def("lt_proc", &FST::lt_proc_len_vec);
}
