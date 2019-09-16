#include <meevax/system/environment.hpp>

int main(const int argc, char const* const* const argv) try
{
  meevax::system::environment program {meevax::system::standard_environment<0>};

  program.configure(argc, argv);

  for (program.open("/dev/stdin"); program.ready(); ) try
  {
    std::cout << "\n> " << std::flush;
    const auto expression {program.read()};
    std::cerr << "\n; read    \t; " << expression << std::endl;

    const auto executable {program.compile(expression)};

    const auto evaluation {program.execute(executable)};
    std::cerr << "; => " << std::flush;
    std::cout << evaluation << std::endl;
  }
  catch (const meevax::system::object& something) // runtime exception generated by user code
  {
    std::cerr << something << std::endl;
    continue;
  }
  catch (const meevax::system::exception& exception) // TODO REMOVE THIS
  {
    std::cerr << exception << std::endl;
    continue; // TODO EXIT IF NOT IN INTERACTIVE MODE
    // return boost::exit_exception_failure;
  }

  return boost::exit_success;
}
catch (const std::exception& error)
{
  std::cout << "\x1b[1;31m" << "unexpected standard exception: \"" << error.what() << "\"" << "\x1b[0m" << std::endl;
  return boost::exit_exception_failure;
}
catch (...)
{
  std::cout << "\x1b[1;31m" << "unexpected exception occurred." << "\x1b[0m" << std::endl;
  return boost::exit_exception_failure;
}

