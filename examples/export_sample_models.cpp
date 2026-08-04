// export_sample_models -- write starter anpcpp JSON models for ANP Studio.
//
// Usage:
//   export_sample_models [output_directory]
// Default output: ./samples
//
// Models are pedagogical reconstructions inspired by well-known AHP/ANP
// examples (SuperDecisions tutorials, Saaty textbook patterns). Judgment
// values are illustrative unless noted as matching a published table.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iostream>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "anpcpp/json_io.hpp"
#include "anpcpp/multiuser.hpp"
#include "anpcpp/network.hpp"
#include "anpcpp/ratings.hpp"

using namespace anpcpp;

namespace fs = std::filesystem;

namespace {

using Pri = std::pair<std::string, double>;

void set_node_pris(AnpNetwork& net,
                   const std::string& wrt,
                   const std::vector<Pri>& pris) {
  if (pris.empty()) return;
  for (const auto& [name, _] : pris) {
    (void)_;
    net.node_connect(wrt, name);
  }
  if (pris.size() == 1) return;
  for (std::size_t i = 0; i < pris.size(); ++i) {
    for (std::size_t j = i + 1; j < pris.size(); ++j) {
      net.set_node_comparison(wrt, pris[i].first, pris[j].first,
                              pris[i].second / pris[j].second);
    }
  }
}

void set_cluster_pris(AnpNetwork& net,
                      const std::string& wrt,
                      const std::vector<Pri>& pris) {
  if (pris.size() < 2) return;
  for (std::size_t i = 0; i < pris.size(); ++i) {
    for (std::size_t j = i + 1; j < pris.size(); ++j) {
      net.set_cluster_comparison(wrt, pris[i].first, pris[j].first,
                                 pris[i].second / pris[j].second);
    }
  }
}

void save(const AnpNetwork& net, const fs::path& dir, const std::string& name) {
  const fs::path path = dir / name;
  save_network_file(net, path.string());
  std::cout << "Wrote " << path << "\n";
}

// ---- 01 Hamburger (SuperDecisions published local priorities) -------------

AnpNetwork build_hamburger() {
  AnpNetwork net;
  net.add_cluster("Advertising");
  net.add_cluster("Quality of Food");
  net.add_cluster("Other");

  for (const char* a : {"McDonalds", "BurgerKing", "Wendys"})
    net.add_node("Alternatives", a);
  for (const char* a : {"Creativity", "Promotion", "Frequency"})
    net.add_node("Advertising", a);
  for (const char* a : {"Nutrition", "Taste", "Portion"})
    net.add_node("Quality of Food", a);
  for (const char* a :
       {"Price", "Location", "Service", "Speed", "Cleanliness", "MenuItem",
        "Takeout", "Reputation"})
    net.add_node("Other", a);

  set_node_pris(net, "McDonalds",
                {{"BurgerKing", 0.8000}, {"Wendys", 0.2000}});
  set_node_pris(net, "McDonalds",
                {{"Creativity", 0.2074},
                 {"Promotion", 0.1298},
                 {"Frequency", 0.6628}});
  set_node_pris(net, "McDonalds",
                {{"Nutrition", 0.3319}, {"Taste", 0.1388}, {"Portion", 0.5293}});
  set_node_pris(net, "McDonalds",
                {{"Price", 0.0329},
                 {"Location", 0.1063},
                 {"Service", 0.0237},
                 {"Speed", 0.0483},
                 {"Cleanliness", 0.3328},
                 {"MenuItem", 0.1593},
                 {"Takeout", 0.0736},
                 {"Reputation", 0.2232}});

  set_node_pris(net, "BurgerKing",
                {{"McDonalds", 0.8333}, {"Wendys", 0.1667}});
  set_node_pris(net, "BurgerKing",
                {{"Creativity", 0.1783},
                 {"Promotion", 0.1120},
                 {"Frequency", 0.7096}});
  set_node_pris(net, "BurgerKing",
                {{"Nutrition", 0.2810}, {"Taste", 0.0720}, {"Portion", 0.6470}});
  set_node_pris(net, "BurgerKing",
                {{"Price", 0.2408},
                 {"Location", 0.2231},
                 {"Service", 0.1418},
                 {"Speed", 0.1407},
                 {"Cleanliness", 0.1096},
                 {"MenuItem", 0.0512},
                 {"Takeout", 0.0506},
                 {"Reputation", 0.0422}});

  set_node_pris(net, "Wendys", {{"McDonalds", 0.7500}, {"BurgerKing", 0.2500}});
  set_node_pris(net, "Wendys",
                {{"Creativity", 0.2810},
                 {"Promotion", 0.0720},
                 {"Frequency", 0.6470}});
  set_node_pris(net, "Wendys",
                {{"Nutrition", 0.6241}, {"Taste", 0.2823}, {"Portion", 0.0936}});
  set_node_pris(net, "Wendys",
                {{"Price", 0.0300},
                 {"Location", 0.1417},
                 {"Service", 0.0648},
                 {"Speed", 0.0641},
                 {"Cleanliness", 0.2756},
                 {"MenuItem", 0.1571},
                 {"Takeout", 0.0589},
                 {"Reputation", 0.2078}});

  set_node_pris(net, "Creativity",
                {{"McDonalds", 0.6141},
                 {"BurgerKing", 0.2685},
                 {"Wendys", 0.1174}});
  set_node_pris(net, "Creativity",
                {{"Promotion", 0.1250}, {"Frequency", 0.8750}});
  set_node_pris(net, "Creativity",
                {{"Location", 0.7095},
                 {"MenuItem", 0.1377},
                 {"Reputation", 0.1528}});

  set_node_pris(net, "Promotion",
                {{"McDonalds", 0.7174},
                 {"BurgerKing", 0.1942},
                 {"Wendys", 0.0884}});
  set_node_pris(net, "Promotion",
                {{"Creativity", 0.3333}, {"Frequency", 0.6667}});
  set_node_pris(net, "Promotion", {{"Price", 0.8333}, {"MenuItem", 0.1667}});

  set_node_pris(net, "Frequency",
                {{"McDonalds", 0.7174},
                 {"BurgerKing", 0.1942},
                 {"Wendys", 0.0884}});
  set_node_pris(net, "Frequency",
                {{"Creativity", 0.5000}, {"Promotion", 0.5000}});
  set_node_pris(net, "Frequency",
                {{"Location", 0.1958},
                 {"MenuItem", 0.3108},
                 {"Reputation", 0.4934}});

  set_node_pris(net, "Nutrition",
                {{"McDonalds", 0.2488},
                 {"BurgerKing", 0.1561},
                 {"Wendys", 0.5951}});
  set_node_pris(net, "Taste",
                {{"McDonalds", 0.2899},
                 {"BurgerKing", 0.1040},
                 {"Wendys", 0.6061}});
  set_node_pris(net, "Portion",
                {{"McDonalds", 0.5989},
                 {"BurgerKing", 0.1262},
                 {"Wendys", 0.2749}});
  set_node_pris(net, "Portion", {{"Price", 0.8571}, {"Takeout", 0.1429}});

  set_node_pris(net, "Price",
                {{"McDonalds", 0.6531},
                 {"BurgerKing", 0.2507},
                 {"Wendys", 0.0962}});
  set_node_pris(net, "Price", {{"Promotion", 0.8333}, {"Frequency", 0.1667}});
  set_node_pris(net, "Price", {{"Nutrition", 0.1667}, {"Portion", 0.8333}});
  set_node_pris(net, "Price", {{"Location", 0.5000}, {"Takeout", 0.5000}});

  set_node_pris(net, "Location",
                {{"McDonalds", 0.6531},
                 {"BurgerKing", 0.2507},
                 {"Wendys", 0.0962}});

  set_node_pris(net, "Service",
                {{"McDonalds", 0.3319},
                 {"BurgerKing", 0.1388},
                 {"Wendys", 0.5293}});
  set_node_pris(net, "Service",
                {{"Location", 0.0981},
                 {"Speed", 0.2857},
                 {"Cleanliness", 0.5181},
                 {"Reputation", 0.0981}});

  set_node_pris(net, "Speed",
                {{"McDonalds", 0.5387},
                 {"BurgerKing", 0.3624},
                 {"Wendys", 0.0989}});
  set_node_pris(net, "Speed",
                {{"Service", 0.1873}, {"Takeout", 0.7313}, {"Reputation", 0.0814}});

  set_node_pris(net, "Cleanliness",
                {{"McDonalds", 0.2500},
                 {"BurgerKing", 0.2500},
                 {"Wendys", 0.5000}});
  set_node_pris(net, "Cleanliness",
                {{"Location", 0.1711}, {"Service", 0.0780}, {"Speed", 0.7509}});

  set_node_pris(net, "MenuItem",
                {{"McDonalds", 0.4934},
                 {"BurgerKing", 0.1958},
                 {"Wendys", 0.3108}});
  set_node_pris(net, "MenuItem",
                {{"Creativity", 0.0780},
                 {"Promotion", 0.1711},
                 {"Frequency", 0.7509}});
  set_node_pris(net, "MenuItem",
                {{"Nutrition", 0.0756}, {"Taste", 0.6952}, {"Portion", 0.2292}});
  set_node_pris(net, "MenuItem",
                {{"Price", 0.1153},
                 {"Location", 0.0526},
                 {"Speed", 0.1946},
                 {"Cleanliness", 0.6375}});

  set_node_pris(net, "Takeout",
                {{"McDonalds", 0.4837},
                 {"BurgerKing", 0.3133},
                 {"Wendys", 0.2029}});
  set_node_pris(net, "Takeout",
                {{"Location", 0.6572}, {"Service", 0.0548}, {"Speed", 0.2880}});

  set_node_pris(net, "Reputation",
                {{"McDonalds", 0.6749},
                 {"BurgerKing", 0.2238},
                 {"Wendys", 0.1012}});
  set_node_pris(net, "Reputation",
                {{"Creativity", 0.0819},
                 {"Promotion", 0.3678},
                 {"Frequency", 0.5503}});
  set_node_pris(net, "Reputation",
                {{"Nutrition", 0.0936}, {"Taste", 0.6241}, {"Portion", 0.2823}});
  set_node_pris(net, "Reputation",
                {{"Price", 0.0627},
                 {"Location", 0.2653},
                 {"Service", 0.0444},
                 {"Speed", 0.0835},
                 {"Cleanliness", 0.2378},
                 {"MenuItem", 0.1929},
                 {"Takeout", 0.0567}});

  set_cluster_pris(net, "Alternatives",
                   {{"Alternatives", 0.2128},
                    {"Advertising", 0.5319},
                    {"Quality of Food", 0.0659},
                    {"Other", 0.1893}});
  set_cluster_pris(net, "Advertising",
                   {{"Alternatives", 0.2956},
                    {"Advertising", 0.2571},
                    {"Other", 0.4473}});
  set_cluster_pris(net, "Quality of Food",
                   {{"Alternatives", 0.5000}, {"Other", 0.5000}});
  set_cluster_pris(net, "Other",
                   {{"Alternatives", 0.1304},
                    {"Advertising", 0.6079},
                    {"Quality of Food", 0.0655},
                    {"Other", 0.1969}});
  return net;
}

AnpNetwork build_tree134() {
  AnpNetwork net;
  net.add_cluster("Goal");
  net.add_cluster("Criteria");
  net.add_node("Goal", "Best Car");
  for (const char* c : {"Cost", "Quality", "Style"}) net.add_node("Criteria", c);
  for (const char* a : {"Civic", "Accord", "CR-V"})
    net.add_node("Alternatives", a);
  for (const char* c : {"Cost", "Quality", "Style"})
    net.node_connect("Best Car", c);
  for (const char* c : {"Cost", "Quality", "Style"})
    for (const char* a : {"Civic", "Accord", "CR-V"}) net.node_connect(c, a);
  net.set_node_comparison("Best Car", "Cost", "Quality", 2.0);
  net.set_node_comparison("Best Car", "Cost", "Style", 4.0);
  net.set_node_comparison("Best Car", "Quality", "Style", 2.0);
  net.set_node_comparison("Cost", "Civic", "Accord", 2.0);
  net.set_node_comparison("Cost", "Civic", "CR-V", 4.0);
  net.set_node_comparison("Cost", "Accord", "CR-V", 2.0);
  net.set_node_comparison("Quality", "Accord", "Civic", 3.0);
  net.set_node_comparison("Quality", "Accord", "CR-V", 2.0);
  net.set_node_comparison("Quality", "CR-V", "Civic", 2.0);
  net.set_node_comparison("Style", "CR-V", "Civic", 2.0);
  net.set_node_comparison("Style", "CR-V", "Accord", 3.0);
  net.set_node_comparison("Style", "Civic", "Accord", 2.0);
  return net;
}

AnpNetwork build_network23() {
  AnpNetwork net;
  net.add_cluster("Criteria");
  for (const char* c : {"Price", "Quality"}) net.add_node("Criteria", c);
  for (const char* a : {"A", "B", "C"}) net.add_node("Alternatives", a);
  net.node_connect("Price", "Quality");
  net.node_connect("Quality", "Price");
  net.set_cluster_comparison("Criteria", "Alternatives", "Criteria", 2.0);
  net.set_cluster_comparison("Alternatives", "Criteria", "Alternatives", 2.0);
  net.set_node_comparison("Price", "A", "B", 2.0);
  net.set_node_comparison("Price", "A", "C", 4.0);
  net.set_node_comparison("Price", "B", "C", 2.0);
  net.set_node_comparison("Quality", "C", "A", 4.0);
  net.set_node_comparison("Quality", "C", "B", 2.0);
  net.set_node_comparison("Quality", "B", "A", 2.0);
  net.set_node_comparison("A", "Price", "Quality", 3.0);
  net.set_node_comparison("B", "Price", "Quality", 1.0);
  net.set_node_comparison("C", "Quality", "Price", 3.0);
  net.set_node_comparison("A", "B", "C", 2.0);
  net.set_node_comparison("B", "A", "C", 2.0);
  net.set_node_comparison("C", "A", "B", 2.0);
  return net;
}

void build_factor_subnet(AnpNetwork& sub,
                         const std::string& f1,
                         const std::string& f2) {
  sub.add_cluster("Factors");
  sub.add_cluster("Alternatives");
  sub.set_alternatives_cluster("Alternatives");
  sub.add_node("Factors", f1);
  sub.add_node("Factors", f2);
  for (const char* a : {"Plan1", "Plan2", "Plan3"})
    sub.add_node("Alternatives", a);
  for (const std::string& f : {f1, f2})
    for (const char* a : {"Plan1", "Plan2", "Plan3"}) sub.node_connect(f, a);
}

AnpNetwork build_benefits_costs() {
  AnpNetwork net(/*create_alts_cluster=*/false);
  net.add_cluster("Goal");
  net.add_cluster("Control");
  net.add_node("Goal", "Choose");
  net.add_node("Control", "Benefits");
  net.add_node("Control", "Costs");
  net.node_connect("Choose", "Benefits");
  net.node_connect("Choose", "Costs");
  net.set_node_comparison("Choose", "Benefits", "Costs", 2.0);

  AnpNetwork& ben = net.subnet("Benefits");
  build_factor_subnet(ben, "Performance", "Convenience");
  ben.set_node_comparison("Performance", "Plan1", "Plan2", 2.0);
  ben.set_node_comparison("Performance", "Plan1", "Plan3", 4.0);
  ben.set_node_comparison("Performance", "Plan2", "Plan3", 2.0);
  ben.set_node_comparison("Convenience", "Plan3", "Plan1", 3.0);
  ben.set_node_comparison("Convenience", "Plan3", "Plan2", 2.0);
  ben.set_node_comparison("Convenience", "Plan2", "Plan1", 2.0);

  AnpNetwork& cost = net.subnet("Costs");
  build_factor_subnet(cost, "Money", "Risk");
  cost.set_node_comparison("Money", "Plan3", "Plan2", 2.0);
  cost.set_node_comparison("Money", "Plan3", "Plan1", 4.0);
  cost.set_node_comparison("Money", "Plan2", "Plan1", 2.0);
  cost.set_node_comparison("Risk", "Plan1", "Plan2", 2.0);
  cost.set_node_comparison("Risk", "Plan1", "Plan3", 3.0);
  cost.set_node_comparison("Risk", "Plan2", "Plan3", 2.0);
  net.node("Costs").set_invert(true);
  return net;
}

AnpNetwork build_ratings_demo() {
  AnpNetwork net;
  net.add_cluster("Goal");
  net.add_node("Goal", "Best");
  net.add_cluster("Criteria");
  for (const char* c : {"Price", "Quality"}) net.add_node("Criteria", c);
  for (const char* a : {"A", "B", "C"}) net.add_node("Alternatives", a);
  net.node_connect("Best", "Price");
  net.node_connect("Best", "Quality");
  net.set_node_comparison("Best", "Price", "Quality", 1.0);
  for (const char* a : {"A", "B", "C"}) net.node_connect("Price", a);
  net.set_node_prioritizer_kind("Price", "Alternatives",
                                NodePrioritizerKind::Ratings);
  {
    auto* rt = net.node("Price").node_ratings("Alternatives");
    rt->set_mode(RatingsPrioritizer::Mode::Categorical);
    rt->set_categories(
        {{"L", "Low", 0.2}, {"M", "Medium", 0.5}, {"H", "High", 1.0}});
    rt->set_rating("A", "H");
    rt->set_rating("B", "M");
    rt->set_rating("C", "L");
  }
  for (const char* a : {"A", "B", "C"}) net.node_connect("Quality", a);
  net.set_node_prioritizer_kind("Quality", "Alternatives",
                                NodePrioritizerKind::Ratings);
  {
    auto* rt = net.node("Quality").node_ratings("Alternatives");
    rt->set_mode(RatingsPrioritizer::Mode::Numeric);
    rt->set_interpreter(DivideByMaxInterpreter{});
    rt->set_value("A", 40.0);
    rt->set_value("B", 80.0);
    rt->set_value("C", 100.0);
  }
  return net;
}

// Saaty-style house purchase (Interfaces / textbook pattern), simplified.
AnpNetwork build_best_house() {
  AnpNetwork net;
  net.add_cluster("Goal");
  net.add_cluster("Criteria");
  net.add_node("Goal", "Best House");
  for (const char* c :
       {"Size", "Location", "Neighborhood", "Age", "Yard", "Modern", "Finance",
        "General"})
    net.add_node("Criteria", c);
  for (const char* a : {"HouseA", "HouseB", "HouseC"})
    net.add_node("Alternatives", a);
  for (anpcpp::AnpNode* c : net.cluster("Criteria").nodes()) {
    net.node_connect("Best House", c->name());
    for (const char* a : {"HouseA", "HouseB", "HouseC"})
      net.node_connect(c->name(), a);
  }
  // Illustrative criteria importance (Size/Location/Neighborhood emphasized).
  net.set_node_comparison("Best House", "Size", "Yard", 3.0);
  net.set_node_comparison("Best House", "Location", "Age", 4.0);
  net.set_node_comparison("Best House", "Neighborhood", "Modern", 3.0);
  net.set_node_comparison("Best House", "Finance", "General", 2.0);
  net.set_node_comparison("Best House", "Size", "Finance", 2.0);
  net.set_node_comparison("Best House", "Location", "Finance", 2.0);
  for (const char* c :
       {"Size", "Location", "Neighborhood", "Age", "Yard", "Modern", "Finance",
        "General"}) {
    net.set_node_comparison(c, "HouseA", "HouseB", 2.0);
    net.set_node_comparison(c, "HouseA", "HouseC", 3.0);
    net.set_node_comparison(c, "HouseB", "HouseC", 2.0);
  }
  // Flip a few so HouseB/C win on some criteria.
  net.set_node_comparison("Finance", "HouseC", "HouseA", 3.0);
  net.set_node_comparison("Age", "HouseB", "HouseA", 2.0);
  net.set_node_comparison("Yard", "HouseC", "HouseB", 2.0);
  return net;
}

AnpNetwork build_school_choice() {
  AnpNetwork net;
  net.add_cluster("Goal");
  net.add_cluster("Criteria");
  net.add_node("Goal", "Best School");
  for (const char* c : {"Learning", "Friends", "SchoolLife", "Vocational",
                        "CollegePrep", "Music", "Athletics"})
    net.add_node("Criteria", c);
  for (const char* a : {"School1", "School2", "School3"})
    net.add_node("Alternatives", a);
  for (anpcpp::AnpNode* c : net.cluster("Criteria").nodes()) {
    net.node_connect("Best School", c->name());
    for (const char* a : {"School1", "School2", "School3"})
      net.node_connect(c->name(), a);
  }
  net.set_node_comparison("Best School", "Learning", "Friends", 3.0);
  net.set_node_comparison("Best School", "Learning", "Athletics", 4.0);
  net.set_node_comparison("Best School", "CollegePrep", "Music", 3.0);
  net.set_node_comparison("Best School", "Vocational", "SchoolLife", 2.0);
  for (const char* c : {"Learning", "Friends", "SchoolLife", "Vocational",
                        "CollegePrep", "Music", "Athletics"}) {
    net.set_node_comparison(c, "School1", "School2", 2.0);
    net.set_node_comparison(c, "School1", "School3", 3.0);
    net.set_node_comparison(c, "School2", "School3", 2.0);
  }
  net.set_node_comparison("Athletics", "School3", "School1", 3.0);
  net.set_node_comparison("Music", "School2", "School1", 2.0);
  return net;
}

AnpNetwork build_vacation() {
  AnpNetwork net;
  net.add_cluster("Goal");
  net.add_cluster("Criteria");
  net.add_node("Goal", "Best Vacation");
  for (const char* c : {"Cost", "Sightseeing", "Adventure", "Relaxation",
                        "Culture", "TravelTime"})
    net.add_node("Criteria", c);
  for (const char* a : {"Paris", "Tokyo", "NewZealand", "Hawaii"})
    net.add_node("Alternatives", a);
  for (anpcpp::AnpNode* c : net.cluster("Criteria").nodes()) {
    net.node_connect("Best Vacation", c->name());
    for (const char* a : {"Paris", "Tokyo", "NewZealand", "Hawaii"})
      net.node_connect(c->name(), a);
  }
  net.set_node_comparison("Best Vacation", "Cost", "TravelTime", 2.0);
  net.set_node_comparison("Best Vacation", "Sightseeing", "Adventure", 2.0);
  net.set_node_comparison("Best Vacation", "Culture", "Relaxation", 2.0);
  net.set_node_comparison("Cost", "Hawaii", "Paris", 2.0);
  net.set_node_comparison("Cost", "Hawaii", "Tokyo", 3.0);
  net.set_node_comparison("Cost", "NewZealand", "Paris", 2.0);
  net.set_node_comparison("Sightseeing", "Paris", "Hawaii", 3.0);
  net.set_node_comparison("Sightseeing", "Tokyo", "Hawaii", 2.0);
  net.set_node_comparison("Adventure", "NewZealand", "Paris", 4.0);
  net.set_node_comparison("Relaxation", "Hawaii", "Tokyo", 3.0);
  net.set_node_comparison("Culture", "Paris", "Hawaii", 4.0);
  net.set_node_comparison("Culture", "Tokyo", "Hawaii", 3.0);
  net.set_node_comparison("TravelTime", "Hawaii", "NewZealand", 3.0);
  return net;
}

AnpNetwork build_job_offer() {
  AnpNetwork net;
  net.add_cluster("Goal");
  net.add_cluster("Criteria");
  net.add_node("Goal", "Best Job");
  for (const char* c :
       {"Salary", "Growth", "Location", "Interest", "Benefits", "Flexibility"})
    net.add_node("Criteria", c);
  for (const char* a : {"OfferA", "OfferB", "OfferC"})
    net.add_node("Alternatives", a);
  for (anpcpp::AnpNode* c : net.cluster("Criteria").nodes()) {
    net.node_connect("Best Job", c->name());
    for (const char* a : {"OfferA", "OfferB", "OfferC"})
      net.node_connect(c->name(), a);
  }
  net.set_node_comparison("Best Job", "Salary", "Location", 2.0);
  net.set_node_comparison("Best Job", "Interest", "Benefits", 3.0);
  net.set_node_comparison("Best Job", "Growth", "Flexibility", 2.0);
  net.set_node_comparison("Salary", "OfferA", "OfferB", 2.0);
  net.set_node_comparison("Salary", "OfferA", "OfferC", 3.0);
  net.set_node_comparison("Growth", "OfferB", "OfferA", 2.0);
  net.set_node_comparison("Location", "OfferC", "OfferA", 3.0);
  net.set_node_comparison("Interest", "OfferB", "OfferC", 2.0);
  net.set_node_comparison("Benefits", "OfferA", "OfferC", 2.0);
  net.set_node_comparison("Flexibility", "OfferC", "OfferB", 2.0);
  return net;
}

AnpNetwork build_car_bcr() {
  // SuperDecisions-style two-level BCR: Benefits / Costs / Risks subnets.
  AnpNetwork net(/*create_alts_cluster=*/false);
  net.add_cluster("Goal");
  net.add_cluster("Merits");
  net.add_node("Goal", "ChooseCar");
  for (const char* m : {"Benefits", "Costs", "Risks"})
    net.add_node("Merits", m);
  net.node_connect("ChooseCar", "Benefits");
  net.node_connect("ChooseCar", "Costs");
  net.node_connect("ChooseCar", "Risks");
  net.set_node_comparison("ChooseCar", "Benefits", "Costs", 2.0);
  net.set_node_comparison("ChooseCar", "Benefits", "Risks", 3.0);
  net.set_node_comparison("ChooseCar", "Costs", "Risks", 2.0);

  auto fill_subnet = [](AnpNetwork& sub, const char* f1, const char* f2,
                        bool invert_parent, AnpNetwork& root,
                        const char* parent) {
    sub.add_cluster("Factors");
    sub.add_cluster("Alternatives");
    sub.set_alternatives_cluster("Alternatives");
    sub.add_node("Factors", f1);
    sub.add_node("Factors", f2);
    for (const char* a : {"American", "European", "Japanese"})
      sub.add_node("Alternatives", a);
    for (const char* f : {f1, f2})
      for (const char* a : {"American", "European", "Japanese"})
        sub.node_connect(f, a);
    if (invert_parent) root.node(parent).set_invert(true);
  };

  AnpNetwork& ben = net.subnet("Benefits");
  fill_subnet(ben, "Style", "Reliability", false, net, "Benefits");
  ben.set_node_comparison("Style", "European", "American", 3.0);
  ben.set_node_comparison("Style", "Japanese", "American", 2.0);
  ben.set_node_comparison("Reliability", "Japanese", "American", 3.0);
  ben.set_node_comparison("Reliability", "Japanese", "European", 2.0);

  AnpNetwork& cost = net.subnet("Costs");
  fill_subnet(cost, "Purchase", "Maintenance", true, net, "Costs");
  cost.set_node_comparison("Purchase", "European", "Japanese", 2.0);
  cost.set_node_comparison("Purchase", "European", "American", 3.0);
  cost.set_node_comparison("Maintenance", "European", "Japanese", 2.0);

  AnpNetwork& risk = net.subnet("Risks");
  fill_subnet(risk, "Resale", "SafetyConcern", true, net, "Risks");
  risk.set_node_comparison("Resale", "American", "European", 2.0);
  risk.set_node_comparison("SafetyConcern", "American", "Japanese", 2.0);
  return net;
}

AnpNetwork build_bocr_launch() {
  AnpNetwork net(/*create_alts_cluster=*/false);
  net.add_cluster("Goal");
  net.add_cluster("BOCR");
  net.add_node("Goal", "LaunchDecision");
  for (const char* m : {"Benefits", "Opportunities", "Costs", "Risks"})
    net.add_node("BOCR", m);
  for (const char* m : {"Benefits", "Opportunities", "Costs", "Risks"})
    net.node_connect("LaunchDecision", m);
  net.set_node_comparison("LaunchDecision", "Benefits", "Costs", 2.0);
  net.set_node_comparison("LaunchDecision", "Opportunities", "Risks", 2.0);
  net.set_node_comparison("LaunchDecision", "Benefits", "Risks", 3.0);

  auto make_sub = [](AnpNetwork& sub, const char* f1, const char* f2) {
    sub.add_cluster("Factors");
    sub.add_cluster("Alternatives");
    sub.set_alternatives_cluster("Alternatives");
    sub.add_node("Factors", f1);
    sub.add_node("Factors", f2);
    for (const char* a : {"FullLaunch", "Pilot", "Delay", "Cancel"})
      sub.add_node("Alternatives", a);
    for (const char* f : {f1, f2})
      for (const char* a : {"FullLaunch", "Pilot", "Delay", "Cancel"})
        sub.node_connect(f, a);
  };

  AnpNetwork& b = net.subnet("Benefits");
  make_sub(b, "Revenue", "Brand");
  b.set_node_comparison("Revenue", "FullLaunch", "Pilot", 3.0);
  b.set_node_comparison("Revenue", "FullLaunch", "Delay", 4.0);
  b.set_node_comparison("Brand", "Pilot", "Cancel", 3.0);

  AnpNetwork& o = net.subnet("Opportunities");
  make_sub(o, "MarketShare", "Partnerships");
  o.set_node_comparison("MarketShare", "FullLaunch", "Delay", 4.0);
  o.set_node_comparison("Partnerships", "Pilot", "Cancel", 3.0);

  AnpNetwork& c = net.subnet("Costs");
  make_sub(c, "Capex", "Opex");
  c.set_node_comparison("Capex", "FullLaunch", "Pilot", 3.0);
  c.set_node_comparison("Opex", "FullLaunch", "Delay", 2.0);
  net.node("Costs").set_invert(true);

  AnpNetwork& r = net.subnet("Risks");
  make_sub(r, "Competitive", "Execution");
  r.set_node_comparison("Competitive", "Delay", "FullLaunch", 3.0);
  r.set_node_comparison("Execution", "FullLaunch", "Pilot", 2.0);
  net.node("Risks").set_invert(true);

  SynthesisOptions syn;
  syn.kind = SynthesisKind::Custom;
  syn.custom_expr = "Benefits * Opportunities / (Costs * Risks)";
  net.set_synthesis_options(syn);
  return net;
}

// Classic small feedback "bridge" pattern: two criteria with mutual dependence.
AnpNetwork build_bridge() {
  AnpNetwork net;
  net.add_cluster("Criteria");
  net.add_node("Criteria", "Economic");
  net.add_node("Criteria", "Social");
  for (const char* a : {"Build", "Repair", "StatusQuo"})
    net.add_node("Alternatives", a);
  net.node_connect("Economic", "Social");
  net.node_connect("Social", "Economic");
  for (const char* c : {"Economic", "Social"})
    for (const char* a : {"Build", "Repair", "StatusQuo"})
      net.node_connect(c, a);
  for (const char* a : {"Build", "Repair", "StatusQuo"}) {
    net.node_connect(a, "Economic");
    net.node_connect(a, "Social");
  }
  net.set_cluster_comparison("Criteria", "Alternatives", "Criteria", 2.0);
  net.set_cluster_comparison("Alternatives", "Criteria", "Alternatives", 1.0);
  net.set_node_comparison("Economic", "Build", "Repair", 2.0);
  net.set_node_comparison("Economic", "Build", "StatusQuo", 4.0);
  net.set_node_comparison("Social", "Repair", "Build", 2.0);
  net.set_node_comparison("Social", "Repair", "StatusQuo", 3.0);
  net.set_node_comparison("Build", "Economic", "Social", 3.0);
  net.set_node_comparison("Repair", "Social", "Economic", 2.0);
  net.set_node_comparison("StatusQuo", "Economic", "Social", 1.0);
  return net;
}

AnpNetwork build_laptop() {
  AnpNetwork net;
  net.add_cluster("Goal");
  net.add_cluster("Criteria");
  net.add_node("Goal", "Best Laptop");
  for (const char* c : {"Performance", "Battery", "Weight", "Price", "Display"})
    net.add_node("Criteria", c);
  for (const char* a : {"ModelX", "ModelY", "ModelZ"})
    net.add_node("Alternatives", a);
  for (anpcpp::AnpNode* c : net.cluster("Criteria").nodes()) {
    net.node_connect("Best Laptop", c->name());
    for (const char* a : {"ModelX", "ModelY", "ModelZ"})
      net.node_connect(c->name(), a);
  }
  net.set_node_comparison("Best Laptop", "Performance", "Weight", 3.0);
  net.set_node_comparison("Best Laptop", "Battery", "Price", 2.0);
  net.set_node_comparison("Best Laptop", "Display", "Weight", 2.0);
  net.set_node_comparison("Performance", "ModelX", "ModelY", 2.0);
  net.set_node_comparison("Performance", "ModelX", "ModelZ", 3.0);
  net.set_node_comparison("Battery", "ModelY", "ModelX", 2.0);
  net.set_node_comparison("Weight", "ModelZ", "ModelX", 3.0);
  net.set_node_comparison("Price", "ModelZ", "ModelY", 2.0);
  net.set_node_comparison("Display", "ModelX", "ModelZ", 2.0);
  return net;
}

AnpNetwork build_supplier() {
  AnpNetwork net;
  net.add_cluster("Goal");
  net.add_cluster("Criteria");
  net.add_node("Goal", "Best Supplier");
  for (const char* c :
       {"Quality", "Cost", "Delivery", "Service", "Flexibility"})
    net.add_node("Criteria", c);
  for (const char* a : {"Supplier1", "Supplier2", "Supplier3", "Supplier4"})
    net.add_node("Alternatives", a);
  for (anpcpp::AnpNode* c : net.cluster("Criteria").nodes()) {
    net.node_connect("Best Supplier", c->name());
    for (const char* a :
         {"Supplier1", "Supplier2", "Supplier3", "Supplier4"})
      net.node_connect(c->name(), a);
  }
  net.set_node_comparison("Best Supplier", "Quality", "Cost", 2.0);
  net.set_node_comparison("Best Supplier", "Delivery", "Service", 2.0);
  net.set_node_comparison("Quality", "Supplier1", "Supplier3", 3.0);
  net.set_node_comparison("Cost", "Supplier4", "Supplier1", 2.0);
  net.set_node_comparison("Delivery", "Supplier2", "Supplier4", 2.0);
  net.set_node_comparison("Service", "Supplier1", "Supplier2", 2.0);
  net.set_node_comparison("Flexibility", "Supplier3", "Supplier4", 2.0);
  return net;
}

AnpNetwork build_project_portfolio() {
  AnpNetwork net;
  net.add_cluster("Goal");
  net.add_cluster("Criteria");
  net.add_node("Goal", "Prioritize Projects");
  for (const char* c :
       {"StrategicFit", "ROI", "Risk", "ResourceNeed", "TimeToValue"})
    net.add_node("Criteria", c);
  for (const char* a : {"ProjectAlpha", "ProjectBeta", "ProjectGamma",
                        "ProjectDelta"})
    net.add_node("Alternatives", a);
  for (anpcpp::AnpNode* c : net.cluster("Criteria").nodes()) {
    net.node_connect("Prioritize Projects", c->name());
    for (const char* a : {"ProjectAlpha", "ProjectBeta", "ProjectGamma",
                          "ProjectDelta"})
      net.node_connect(c->name(), a);
  }
  net.set_node_comparison("Prioritize Projects", "StrategicFit", "ROI", 2.0);
  net.set_node_comparison("Prioritize Projects", "ROI", "Risk", 2.0);
  // Risk inverted in spirit via judgments (higher risk worse for project).
  net.set_node_comparison("Risk", "ProjectDelta", "ProjectAlpha", 3.0);
  net.set_node_comparison("StrategicFit", "ProjectAlpha", "ProjectGamma", 3.0);
  net.set_node_comparison("ROI", "ProjectBeta", "ProjectDelta", 2.0);
  net.set_node_comparison("ResourceNeed", "ProjectDelta", "ProjectAlpha", 2.0);
  net.set_node_comparison("TimeToValue", "ProjectBeta", "ProjectGamma", 2.0);
  return net;
}

AnpNetwork build_smartphone_ratings() {
  AnpNetwork net;
  net.add_cluster("Goal");
  net.add_cluster("Criteria");
  net.add_node("Goal", "Best Phone");
  for (const char* c : {"Camera", "Battery", "Price", "Storage"})
    net.add_node("Criteria", c);
  for (const char* a : {"PhoneX", "PhoneY", "PhoneZ"})
    net.add_node("Alternatives", a);
  net.node_connect("Best Phone", "Camera");
  net.node_connect("Best Phone", "Battery");
  net.node_connect("Best Phone", "Price");
  net.node_connect("Best Phone", "Storage");
  net.set_node_comparison("Best Phone", "Camera", "Price", 2.0);
  net.set_node_comparison("Best Phone", "Battery", "Storage", 2.0);

  // Pairwise for camera.
  for (const char* a : {"PhoneX", "PhoneY", "PhoneZ"})
    net.node_connect("Camera", a);
  net.set_node_comparison("Camera", "PhoneX", "PhoneY", 2.0);
  net.set_node_comparison("Camera", "PhoneX", "PhoneZ", 3.0);

  // Ratings for battery (mAh-like), price (categorical), storage (GB / max).
  for (const char* a : {"PhoneX", "PhoneY", "PhoneZ"})
    net.node_connect("Battery", a);
  net.set_node_prioritizer_kind("Battery", "Alternatives",
                                NodePrioritizerKind::Ratings);
  {
    auto* rt = net.node("Battery").node_ratings("Alternatives");
    rt->set_mode(RatingsPrioritizer::Mode::Numeric);
    rt->set_interpreter(DivideByMaxInterpreter{});
    rt->set_value("PhoneX", 4000);
    rt->set_value("PhoneY", 5000);
    rt->set_value("PhoneZ", 4500);
  }
  for (const char* a : {"PhoneX", "PhoneY", "PhoneZ"})
    net.node_connect("Price", a);
  net.set_node_prioritizer_kind("Price", "Alternatives",
                                NodePrioritizerKind::Ratings);
  {
    auto* rt = net.node("Price").node_ratings("Alternatives");
    rt->set_mode(RatingsPrioritizer::Mode::Categorical);
    rt->set_categories(
        {{"Exp", "Expensive", 0.2}, {"Mid", "Mid", 0.5}, {"Val", "Value", 1.0}});
    rt->set_rating("PhoneX", "Exp");
    rt->set_rating("PhoneY", "Mid");
    rt->set_rating("PhoneZ", "Val");
  }
  for (const char* a : {"PhoneX", "PhoneY", "PhoneZ"})
    net.node_connect("Storage", a);
  net.set_node_prioritizer_kind("Storage", "Alternatives",
                                NodePrioritizerKind::Ratings);
  {
    auto* rt = net.node("Storage").node_ratings("Alternatives");
    rt->set_mode(RatingsPrioritizer::Mode::Numeric);
    rt->set_interpreter(DivideByConstantInterpreter{512.0});
    rt->set_value("PhoneX", 256);
    rt->set_value("PhoneY", 512);
    rt->set_value("PhoneZ", 128);
  }
  return net;
}

AnpNetwork build_water_reservoir() {
  // Simplified single-network policy example inspired by SuperDecisions
  // "optimal water level" style models.
  AnpNetwork net;
  net.add_cluster("Criteria");
  for (const char* c : {"FloodControl", "WaterSupply", "Environment", "Recreation"})
    net.add_node("Criteria", c);
  for (const char* a : {"LowLevel", "MediumLevel", "HighLevel"})
    net.add_node("Alternatives", a);
  for (anpcpp::AnpNode* c : net.cluster("Criteria").nodes())
    for (const char* a : {"LowLevel", "MediumLevel", "HighLevel"})
      net.node_connect(c->name(), a);
  net.set_node_comparison("FloodControl", "LowLevel", "HighLevel", 4.0);
  net.set_node_comparison("FloodControl", "MediumLevel", "HighLevel", 2.0);
  net.set_node_comparison("WaterSupply", "HighLevel", "LowLevel", 4.0);
  net.set_node_comparison("Environment", "MediumLevel", "HighLevel", 2.0);
  net.set_node_comparison("Recreation", "HighLevel", "LowLevel", 3.0);
  // Mild feedback among criteria.
  net.node_connect("FloodControl", "Environment");
  net.node_connect("Environment", "Recreation");
  net.set_node_comparison("FloodControl", "Environment", "FloodControl", 1.0);
  return net;
}

// ---- Multi-user samples (geometric pairwise / arithmetic ratings) ---------

AnpNetwork build_multiuser_pairwise_ahp() {
  // Hand-check: A vs B ratios 2, 8, 1/2 → geo mean 2.
  AnpNetwork net(false);
  net.set_name("Multi-user pairwise AHP");
  net.set_description(
      "Three participants; Goal→Alternatives pairwise. Overall average uses "
      "geometric mean of ratios.");
  net.add_cluster("Criteria");
  net.add_cluster("Alternatives");
  net.set_alternatives_cluster("Alternatives");
  net.add_node("Criteria", "Goal");
  for (const char* a : {"A", "B", "C"}) net.add_node("Alternatives", a);

  net.add_participant("alice", "Alice Chen", "alice@example.com");
  net.add_participant("bob", "Bob Rivera", "bob@example.com");
  net.add_participant("carol", "Carol Ng", "carol@example.com");
  net.add_judgment_group("exec", "Executives", {"alice", "bob"});

  for (const char* a : {"A", "B", "C"}) net.node_connect("Goal", a);

  auto fill = [&](const char* uid, double ab, double ac, double bc) {
    net.set_node_comparison_for(uid, "Goal", "A", "B", ab);
    net.set_node_comparison_for(uid, "Goal", "A", "C", ac);
    net.set_node_comparison_for(uid, "Goal", "B", "C", bc);
  };
  fill("alice", 2.0, 3.0, 1.0);
  fill("bob", 8.0, 3.0, 1.0);
  fill("carol", 0.5, 3.0, 1.0);

  net.set_judgment_session({JudgmentScopeKind::Average, {}});
  net.rebuild_effective_judgments();
  return net;
}

AnpNetwork build_multiuser_ratings() {
  // A scores 0.2 and 0.8 → arithmetic mean 0.5.
  AnpNetwork net(false);
  net.set_name("Multi-user ratings");
  net.set_description(
      "Numeric ratings with Identity interpreter; overall average uses "
      "arithmetic mean of scores.");
  net.add_cluster("Criteria");
  net.add_cluster("Alternatives");
  net.set_alternatives_cluster("Alternatives");
  net.add_node("Criteria", "Quality");
  for (const char* a : {"A", "B", "C"}) net.add_node("Alternatives", a);

  net.add_participant("alice", "Alice");
  net.add_participant("bob", "Bob");
  net.add_participant("carol", "Carol");
  net.add_participant("diego", "Diego");  // no votes on A — omitted from mean

  for (const char* a : {"A", "B", "C"}) net.node_connect("Quality", a);
  net.set_node_prioritizer_kind("Quality", "Alternatives",
                                NodePrioritizerKind::Ratings);
  {
    auto* slot = net.node("Quality").node_prioritizer("Alternatives");
    slot->ratings.set_mode(RatingsPrioritizer::Mode::Numeric);
    slot->ratings.set_interpreter(IdentityInterpreter{});
    slot->sync_ratings_scale_to_users();
  }

  net.set_node_rating_value_for("alice", "Quality", "A", 0.2);
  net.set_node_rating_value_for("bob", "Quality", "A", 0.8);
  net.set_node_rating_value_for("carol", "Quality", "A", 0.5);
  net.set_node_rating_value_for("alice", "Quality", "B", 0.4);
  net.set_node_rating_value_for("bob", "Quality", "B", 0.6);
  net.set_node_rating_value_for("carol", "Quality", "B", 0.5);
  net.set_node_rating_value_for("alice", "Quality", "C", 0.9);
  net.set_node_rating_value_for("bob", "Quality", "C", 0.7);

  net.set_judgment_session({JudgmentScopeKind::Average, {}});
  net.rebuild_effective_judgments();
  return net;
}

AnpNetwork build_multiuser_mixed() {
  AnpNetwork net(false);
  net.set_name("Multi-user mixed pairwise + ratings");
  net.set_description(
      "Criteria compared pairwise (geometric mean); alts rated under Cost "
      "(arithmetic mean). Executives group = Alice+Bob.");
  net.add_cluster("Criteria");
  net.add_cluster("Alternatives");
  net.set_alternatives_cluster("Alternatives");
  net.add_node("Criteria", "Goal");
  net.add_node("Criteria", "Cost");
  net.add_node("Criteria", "Quality");
  for (const char* a : {"Plan1", "Plan2", "Plan3"})
    net.add_node("Alternatives", a);

  net.add_participant("alice", "Alice");
  net.add_participant("bob", "Bob");
  net.add_participant("carol", "Carol");
  net.add_judgment_group("exec", "Executives", {"alice", "bob"});

  net.node_connect("Goal", "Cost");
  net.node_connect("Goal", "Quality");
  net.set_node_comparison_for("alice", "Goal", "Cost", "Quality", 2.0);
  net.set_node_comparison_for("bob", "Goal", "Cost", "Quality", 0.5);
  net.set_node_comparison_for("carol", "Goal", "Cost", "Quality", 1.0);

  for (const char* a : {"Plan1", "Plan2", "Plan3"}) {
    net.node_connect("Cost", a);
    net.node_connect("Quality", a);
  }
  net.set_node_prioritizer_kind("Cost", "Alternatives",
                                NodePrioritizerKind::Ratings);
  {
    auto* slot = net.node("Cost").node_prioritizer("Alternatives");
    slot->ratings.set_mode(RatingsPrioritizer::Mode::Numeric);
    slot->ratings.set_interpreter(IdentityInterpreter{});
    slot->sync_ratings_scale_to_users();
  }
  net.set_node_rating_value_for("alice", "Cost", "Plan1", 0.3);
  net.set_node_rating_value_for("bob", "Cost", "Plan1", 0.5);
  net.set_node_rating_value_for("carol", "Cost", "Plan1", 0.4);
  net.set_node_rating_value_for("alice", "Cost", "Plan2", 0.6);
  net.set_node_rating_value_for("bob", "Cost", "Plan2", 0.6);
  net.set_node_rating_value_for("carol", "Cost", "Plan2", 0.6);

  net.set_node_comparison_for("alice", "Quality", "Plan1", "Plan2", 3.0);
  net.set_node_comparison_for("bob", "Quality", "Plan1", "Plan2", 1.0 / 3.0);
  net.set_node_comparison_for("carol", "Quality", "Plan1", "Plan2", 1.0);
  net.set_node_comparison_for("alice", "Quality", "Plan1", "Plan3", 2.0);
  net.set_node_comparison_for("bob", "Quality", "Plan1", "Plan3", 2.0);
  net.set_node_comparison_for("carol", "Quality", "Plan1", "Plan3", 2.0);
  net.set_node_comparison_for("alice", "Quality", "Plan2", "Plan3", 1.0);
  net.set_node_comparison_for("bob", "Quality", "Plan2", "Plan3", 1.0);
  net.set_node_comparison_for("carol", "Quality", "Plan2", "Plan3", 1.0);

  net.set_judgment_session({JudgmentScopeKind::Average, {}});
  net.rebuild_effective_judgments();
  return net;
}

AnpNetwork build_multiuser_partial() {
  AnpNetwork net(false);
  net.set_name("Multi-user partial coverage");
  net.set_description(
      "Sparse judgments: Carol has no A vs B; Diego empty. Aggregation skips "
      "missing cells.");
  net.add_cluster("Criteria");
  net.add_cluster("Alternatives");
  net.set_alternatives_cluster("Alternatives");
  net.add_node("Criteria", "Goal");
  for (const char* a : {"A", "B", "C"}) net.add_node("Alternatives", a);

  net.add_participant("alice", "Alice");
  net.add_participant("bob", "Bob");
  net.add_participant("carol", "Carol");
  net.add_participant("diego", "Diego");

  for (const char* a : {"A", "B", "C"}) net.node_connect("Goal", a);

  net.set_node_comparison_for("alice", "Goal", "A", "B", 4.0);
  net.set_node_comparison_for("bob", "Goal", "A", "B", 1.0);
  // carol skips A vs B
  net.set_node_comparison_for("alice", "Goal", "A", "C", 2.0);
  net.set_node_comparison_for("bob", "Goal", "A", "C", 2.0);
  net.set_node_comparison_for("carol", "Goal", "A", "C", 2.0);
  net.set_node_comparison_for("alice", "Goal", "B", "C", 1.0);
  net.set_node_comparison_for("bob", "Goal", "B", "C", 1.0);
  net.set_node_comparison_for("carol", "Goal", "B", "C", 1.0);

  net.set_judgment_session({JudgmentScopeKind::Average, {}});
  net.rebuild_effective_judgments();
  return net;
}

// Hamburger ANP with five full-vote participants (same structure as sample 01).
AnpNetwork build_hamburger_multiuser() {
  AnpNetwork net = build_hamburger();
  net.set_name("Multi-user Hamburger market share");
  net.set_description(
      "Same SuperDecisions-style Hamburger network as sample 01, with five "
      "participants who each cast a full set of pairwise votes: Standard "
      "(tutorial local priorities), Wendy's / McDonald's / Burger King lovers "
      "(brand bias plus mild deterministic noise for interesting sensitivity), "
      "and Chaos (extreme scrambled priorities). Group average uses geometric "
      "mean of ratios.");

  net.add_participant("standard", "Standard", "");
  net.add_participant("wendy_fan", "Wendy's Lover", "");
  net.add_participant("mcd_fan", "McDonald's Lover", "");
  net.add_participant("bk_fan", "Burger King Lover", "");
  net.add_participant("chaos", "Chaos", "");

  auto is_restaurant = [](const std::string& name) {
    return name == "McDonalds" || name == "BurgerKing" || name == "Wendys";
  };

  struct Baseline {
    bool cluster = false;
    std::string wrt;
    std::string dest_cluster;
    std::vector<std::string> alts;
    Vector pris;
  };
  std::vector<Baseline> baselines;
  for (AnpNode* n : net.nodes()) {
    for (AnpCluster* dc : net.clusters()) {
      PairwiseJudgments* pw = n->node_pairwise(dc->name());
      if (pw == nullptr || pw->size() < 2) continue;
      baselines.push_back(
          {false, n->name(), dc->name(), pw->alternatives(), pw->priorities()});
    }
  }
  for (AnpCluster* c : net.clusters()) {
    PairwiseJudgments& pw = c->cluster_pairwise();
    if (pw.size() < 2) continue;
    baselines.push_back(
        {true, c->name(), {}, pw.alternatives(), pw.priorities()});
  }

  auto transform = [&](const std::string& profile, const std::string& key,
                       const std::vector<std::string>& alts,
                       const Vector& base) {
    std::vector<double> p(alts.size());
    for (std::size_t i = 0; i < alts.size(); ++i) p[i] = base[i];

    if (profile == "standard") return p;

    if (profile == "chaos") {
      std::uint64_t h = static_cast<std::uint64_t>(std::hash<std::string>{}(key));
      for (std::size_t i = 0; i < p.size(); ++i) {
        const std::uint64_t nibble = (h >> ((i * 7) % 60)) & 0x3Fu;
        const double raw = 0.04 + static_cast<double>(nibble) / 63.0;
        p[i] = std::pow(raw, 2.5);
        h = h * 6364136223846793005ULL + 1ULL;
      }
    } else {
      // Brand fans: start from tutorial weights, add mild keyed noise, then
      // boost the favorite restaurant when that set is being compared.
      std::uint64_t h = static_cast<std::uint64_t>(
          std::hash<std::string>{}(key + "|" + profile));
      for (std::size_t i = 0; i < p.size(); ++i) {
        const std::uint64_t nibble = (h >> ((i * 5) % 60)) & 0x3Fu;
        const double u = static_cast<double>(nibble) / 63.0;  // [0, 1]
        constexpr double kAmp = 0.28;  // ±28% multiplicative noise
        p[i] = std::max(1e-9, p[i] * (1.0 + kAmp * (2.0 * u - 1.0)));
        h = h * 6364136223846793005ULL + 1ULL;
      }

      const char* fav =
          profile == "wendy_fan"
              ? "Wendys"
              : (profile == "mcd_fan" ? "McDonalds" : "BurgerKing");
      const bool restaurant_set =
          !alts.empty() &&
          std::all_of(alts.begin(), alts.end(), is_restaurant);
      if (restaurant_set) {
        const auto fav_it = std::find(alts.begin(), alts.end(), fav);
        if (fav_it != alts.end()) {
          const std::size_t fi =
              static_cast<std::size_t>(fav_it - alts.begin());
          p[fi] *= (alts.size() == 2) ? 5.0 : 3.5;
        }
      }
    }

    double sum = std::accumulate(p.begin(), p.end(), 0.0);
    if (sum <= 0.0) {
      for (double& v : p) v = 1.0 / static_cast<double>(p.size());
    } else {
      for (double& v : p) v /= sum;
    }
    return p;
  };

  const std::vector<std::pair<std::string, std::string>> users = {
      {"standard", "standard"}, {"wendy_fan", "wendy_fan"},
      {"mcd_fan", "mcd_fan"},   {"bk_fan", "bk_fan"},
      {"chaos", "chaos"},
  };

  for (const auto& [uid, profile] : users) {
    for (const Baseline& b : baselines) {
      const std::string key = b.wrt + "|" + b.dest_cluster;
      const std::vector<double> p =
          transform(profile, key + "|" + uid, b.alts, b.pris);
      for (std::size_t i = 0; i < b.alts.size(); ++i) {
        for (std::size_t j = i + 1; j < b.alts.size(); ++j) {
          const double ratio = p[i] / p[j];
          if (b.cluster) {
            net.set_cluster_comparison_for(uid, b.wrt, b.alts[i], b.alts[j],
                                           ratio);
          } else {
            net.set_node_comparison_for(uid, b.wrt, b.alts[i], b.alts[j],
                                        ratio);
          }
        }
      }
    }
  }

  net.set_judgment_session({JudgmentScopeKind::Average, {}});
  net.rebuild_effective_judgments();
  return net;
}

}  // namespace

int main(int argc, char** argv) {
  fs::path out = (argc > 1) ? fs::path(argv[1]) : fs::path("samples");
  fs::create_directories(out);

  save(build_hamburger(), out, "01_hamburger_marketshare.anpstudio");
  save(build_tree134(), out, "02_ahp_best_car.anpstudio");
  save(build_network23(), out, "03_anp_network23_feedback.anpstudio");
  save(build_benefits_costs(), out, "04_bcr_benefits_costs_plans.anpstudio");
  save(build_ratings_demo(), out, "05_ratings_price_quality.anpstudio");
  save(build_best_house(), out, "06_ahp_best_house.anpstudio");
  save(build_school_choice(), out, "07_ahp_school_choice.anpstudio");
  save(build_vacation(), out, "08_ahp_vacation.anpstudio");
  save(build_job_offer(), out, "09_ahp_job_offer.anpstudio");
  save(build_car_bcr(), out, "10_bcr_car_purchase.anpstudio");
  save(build_bocr_launch(), out, "11_bocr_product_launch.anpstudio");
  save(build_bridge(), out, "12_anp_bridge_feedback.anpstudio");
  save(build_laptop(), out, "13_ahp_laptop.anpstudio");
  save(build_supplier(), out, "14_ahp_supplier_selection.anpstudio");
  save(build_project_portfolio(), out, "15_ahp_project_portfolio.anpstudio");
  save(build_smartphone_ratings(), out, "16_ahp_smartphone_ratings.anpstudio");
  save(build_water_reservoir(), out, "17_anp_water_reservoir.anpstudio");
  save(build_multiuser_pairwise_ahp(), out, "18_multiuser_pairwise_ahp.anpstudio");
  save(build_multiuser_ratings(), out, "19_multiuser_ratings.anpstudio");
  save(build_multiuser_mixed(), out, "20_multiuser_mixed.anpstudio");
  save(build_multiuser_partial(), out, "21_multiuser_partial.anpstudio");
  save(build_hamburger_multiuser(), out, "22_multiuser_hamburger.anpstudio");

  std::cout << "Done. " << out << " contains starter models for ANP Studio.\n";
  return 0;
}
