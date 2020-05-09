// #define THE_ONLY_SUBSET_OF_THE_EMPTY_SET_IS_ITSELF true

#include <meevax/kernel/syntactic_continuation.hpp>

int main(const int argc, char const* const* const argv) try
{
  meevax::kernel::syntactic_continuation ice { meevax::kernel::layer<2> };

  /*============================================================================
  *
  * The environment system includes a command line option parser. The parser is
  * internally called the "configurator" and is primarily responsible for
  * changing the behavior of the environment.
  *
  *========================================================================== */
  ice.configure(argc, argv);

  ice.write(
    "; console\t; You have control of root syntactic-continuation.\n");

  for (const auto prompt { "\n> " }; ice.ready(); ) try
  {
    ice.write(prompt);

    const auto expression { ice.read() };

    std::cout << "\n";

    ice.write_to(ice.current_debug_port(),
      ice.header("read"), expression, "\n");

    const auto evaluation { ice.evaluate(expression) };

    std::cout << evaluation << std::endl;
  }
  catch (const meevax::kernel::object& something) // runtime exception generated by user code
  {
    std::cerr << something << std::endl;
    continue;
  }
  catch (const meevax::kernel::exception& exception) // TODO REMOVE THIS
  {
    std::cerr << exception << std::endl;
    continue; // TODO EXIT IF NOT IN INTERACTIVE MODE
    // return boost::exit_exception_failure;
  }

  std::cerr << "\n"
            << "; console\t; I have control of root syntactic-continuation.\n"
            << std::endl;

  if (true)
  {
    std::cerr << "Test/1 - Write/Read Invariance" << std::endl;

    std::stringstream text_port {"'(+ 1 2 3)"};

    text_port >> ice;
    text_port << ice;
    text_port >> ice;
    std::cout << ice;

    std::cerr << std::endl;
  }

  if (true)
  {
    std::cerr << "Test/2 - Tagged Pointers" << std::endl;

    auto value {meevax::kernel::make<float>(3.14)};

    auto x {value.as<float>()};
    std::cout << "; pointer\t; " << x << std::endl;;

    auto y {value.as<int>()};
    std::cout << "; pointer\t; " << y << std::endl;;

    std::cerr << std::endl;
  }

  return boost::exit_success;
}
catch (const meevax::kernel::exception& error)
{
  std::cerr << error << std::endl;
  return boost::exit_exception_failure;
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

