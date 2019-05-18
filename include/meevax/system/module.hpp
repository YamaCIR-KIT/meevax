#ifndef INCLUDED_MEEVAX_SYSTEM_MODULE_HPP
#define INCLUDED_MEEVAX_SYSTEM_MODULE_HPP

#include <functional> // std::invoke
#include <unordered_map>

#include <meevax/posix/linker.hpp>
#include <meevax/system/machine.hpp>
#include <meevax/system/reader.hpp>

namespace meevax::system
{
  struct module
    : public std::unordered_map<std::string, objective> // XXX public?
    , public reader<module>
  {
    const std::string name;

    machine execute;

    template <typename... Ts>
    module(const std::string& name, Ts&&... args)
      : std::unordered_map<std::string, objective> {std::forward<Ts>(args)...}
      , name {name}
    {
      std::cerr << "constructing module \"" << name << "\" => ";
      std::cerr << "done." << std::endl;
    }

  public: // reader interface
    auto ready() const noexcept
    {
      return static_cast<bool>(*this); // TODO MORE
    }

  public: // virtual machine interface
    template <typename T, typename... Ts>
    decltype(auto) define(const std::string& name, Ts&&... args)
    {
      return execute.define(intern(name), make<T>(name, std::forward<Ts>(args)...));
    }

    template <typename... Ts>
    decltype(auto) compile(Ts&&... args) // XXX こんなものを提供しなきゃいけないのがそもそもおかしい
    {
      return execute.compile(std::forward<Ts>(args)...);
    }

    template <typename... Ts>
    decltype(auto) begin(Ts&&... args) // XXX こんなものを提供しなきゃいけないのがそもそもおかしい
    {
      return execute.begin(std::forward<Ts>(args)...);
    }

    const auto& intern(const std::string& s)
    {
      if (auto iter {find(s)}; iter != std::unordered_map<std::string, objective>::end())
      {
        return iter->second;
      }
      else
      {
        iter = emplace(s, make<symbol>(s)).first;
        return iter->second;
      }
    }

  public:
    template <typename... Ts>
    decltype(auto) load(Ts&&... args) noexcept(false)
    {
      if (module loader {"unnamed-loader"}; loader.open(std::forward<Ts>(args)...), loader.ready())
      {
        loader.merge(*this);
        loader.execute.env = execute.env;

        while (loader.ready())
        {
          const auto expression {loader.read()};
          const auto executable {loader.compile(expression)};
          const auto evaluation {loader.execute(executable)};
        }

        merge(loader);

        std::cerr << "[debug] " << std::distance(loader.execute.env, execute.env) << " expression defined" << std::endl;
        execute.env = loader.execute.env;

        return true_v;
      }
      else
      {
        std::cerr << "[debug] failed to open file" << std::endl; // TODO CONVERT TO EXCEPTION
        return false_v;
      }
    }
  };

  std::ostream& operator<<(std::ostream&, const module&);
} // namespace meevax::system

#endif // INCLUDED_MEEVAX_SYSTEM_MODULE_HPP

