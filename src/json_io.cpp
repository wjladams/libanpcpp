#include "anpcpp/json_io.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

#include <nlohmann/json.hpp>

namespace anpcpp {
namespace {

using json = nlohmann::json;

// Serialize a pairwise table: alternative names plus the full n×n matrix
// (including reciprocals on the lower triangle).
json pairwise_to_json(const PairwiseJudgments& pw) {
  json j;
  j["alternatives"] = pw.alternatives();
  json matrix = json::array();
  for (std::size_t i = 0; i < pw.size(); ++i) {
    json row = json::array();
    for (std::size_t j = 0; j < pw.size(); ++j) {
      row.push_back(pw.comparison(i, j));
    }
    matrix.push_back(row);
  }
  j["matrix"] = matrix;
  return j;
}

void pairwise_from_json(PairwiseJudgments& pw, const json& j) {
  const auto alts = j.at("alternatives").get<std::vector<std::string>>();
  for (const auto& a : alts) {
    pw.add_alternative(a, /*ignore_existing=*/true);
  }
  if (!j.contains("matrix")) return;
  // Only upper-triangle entries are stored; set_comparison fills reciprocals.
  const auto& matrix = j.at("matrix");
  for (std::size_t i = 0; i < alts.size(); ++i) {
    for (std::size_t j = i + 1; j < alts.size(); ++j) {
      const double v = matrix.at(i).at(j).get<double>();
      pw.set_comparison(i, j, v);
    }
  }
}

json interpreter_to_json(const ScoreInterpreter& interpreter) {
  return std::visit(
      [](const auto& interp) -> json {
        using T = std::decay_t<decltype(interp)>;
        if constexpr (std::is_same_v<T, IdentityInterpreter>) {
          return {{"kind", "identity"}};
        } else if constexpr (std::is_same_v<T, DivideByMaxInterpreter>) {
          return {{"kind", "divide_by_max"}};
        } else if constexpr (std::is_same_v<T, DivideByConstantInterpreter>) {
          return {{"kind", "divide_by_constant"},
                  {"constant", interp.constant}};
        } else if constexpr (std::is_same_v<T, MinMaxNormalizeInterpreter>) {
          return {{"kind", "minmax"}};
        } else if constexpr (std::is_same_v<T, PiecewiseLinearInterpreter>) {
          json knots = json::array();
          for (const auto& [x, y] : interp.knots) {
            knots.push_back(json::array({x, y}));
          }
          return {{"kind", "piecewise_linear"}, {"knots", knots}};
        }
      },
      interpreter);
}

ScoreInterpreter interpreter_from_json(const json& j) {
  const std::string kind = j.value("kind", "identity");
  if (kind == "identity") {
    return IdentityInterpreter{};
  }
  if (kind == "divide_by_max") {
    return DivideByMaxInterpreter{};
  }
  if (kind == "divide_by_constant") {
    return DivideByConstantInterpreter{j.at("constant").get<double>()};
  }
  if (kind == "minmax") {
    return MinMaxNormalizeInterpreter{};
  }
  if (kind == "piecewise_linear") {
    PiecewiseLinearInterpreter pl;
    for (const auto& knot : j.at("knots")) {
      pl.knots.emplace_back(knot.at(0).get<double>(), knot.at(1).get<double>());
    }
    return pl;
  }
  throw std::invalid_argument("unknown score interpreter kind: " + kind);
}

json ratings_to_json(const RatingsPrioritizer& rt) {
  json j;
  j["type"] = "ratings";
  j["alternatives"] = rt.alternatives();
  j["mode"] =
      rt.mode() == RatingsPrioritizer::Mode::Categorical ? "categorical"
                                                         : "numeric";
  if (rt.mode() == RatingsPrioritizer::Mode::Categorical) {
    json cats = json::array();
    for (const auto& c : rt.categories()) {
      cats.push_back(
          {{"id", c.id}, {"label", c.label}, {"value", c.value}});
    }
    j["categories"] = cats;
    json ratings = json::object();
    for (const auto& alt : rt.alternatives()) {
      const auto r = rt.rating(alt);
      if (r.has_value()) {
        ratings[alt] = *r;
      }
    }
    j["ratings"] = ratings;
  } else {
    j["interpreter"] = interpreter_to_json(rt.interpreter());
    json values = json::object();
    for (const auto& alt : rt.alternatives()) {
      const auto v = rt.value(alt);
      if (v.has_value()) {
        values[alt] = *v;
      }
    }
    j["values"] = values;
  }
  return j;
}

void ratings_from_json(RatingsPrioritizer& rt, const json& j) {
  const auto alts = j.at("alternatives").get<std::vector<std::string>>();
  for (const auto& a : alts) {
    rt.add_alternative(a, /*ignore_existing=*/true);
  }
  const std::string mode = j.value("mode", "numeric");
  if (mode == "categorical") {
    rt.set_mode(RatingsPrioritizer::Mode::Categorical);
    std::vector<RatingCategory> cats;
    if (j.contains("categories")) {
      for (const auto& cj : j.at("categories")) {
        cats.push_back(RatingCategory{
            cj.at("id").get<std::string>(),
            cj.value("label", cj.at("id").get<std::string>()),
            cj.at("value").get<double>(),
        });
      }
    }
    rt.set_categories(std::move(cats));
    if (j.contains("ratings")) {
      for (auto it = j.at("ratings").begin(); it != j.at("ratings").end();
           ++it) {
        rt.set_rating(it.key(), it.value().get<std::string>());
      }
    }
  } else {
    rt.set_mode(RatingsPrioritizer::Mode::Numeric);
    if (j.contains("interpreter")) {
      rt.set_interpreter(interpreter_from_json(j.at("interpreter")));
    } else {
      rt.set_interpreter(IdentityInterpreter{});
    }
    if (j.contains("values")) {
      for (auto it = j.at("values").begin(); it != j.at("values").end();
           ++it) {
        rt.set_value(it.key(), it.value().get<double>());
      }
    }
  }
}

json prioritizer_to_json(const NodePrioritizerSlot& slot) {
  if (slot.kind == NodePrioritizerKind::Pairwise) {
    json j = pairwise_to_json(slot.pairwise);
    j["type"] = "pairwise";
    return j;
  }
  return ratings_to_json(slot.ratings);
}

void connect_alts(AnpNetwork& net,
                  AnpNode& node,
                  const std::vector<std::string>& alts) {
  for (const std::string& dest_name : alts) {
    if (net.find_node(dest_name) != nullptr) {
      net.node_connect(node.name(), dest_name);
    }
  }
}

void load_prioritizer_entry(AnpNetwork& net,
                            AnpNode& node,
                            const std::string& dest_cluster,
                            const json& entry) {
  const auto alts =
      entry.at("alternatives").get<std::vector<std::string>>();
  connect_alts(net, node, alts);

  const std::string type = entry.value("type", "pairwise");
  if (type == "ratings") {
    node.set_node_prioritizer_kind(dest_cluster, NodePrioritizerKind::Ratings);
    RatingsPrioritizer* rt = node.node_ratings(dest_cluster);
    if (rt == nullptr) {
      throw std::logic_error("missing ratings after kind switch");
    }
    // Slot already has alts from connect; refill from JSON cleanly.
    *rt = RatingsPrioritizer{};
    ratings_from_json(*rt, entry);
  } else {
    // Ensure pairwise (default after connect).
    if (node.node_prioritizer_kind(dest_cluster) !=
        NodePrioritizerKind::Pairwise) {
      node.set_node_prioritizer_kind(dest_cluster,
                                     NodePrioritizerKind::Pairwise);
    }
    PairwiseJudgments* pw = node.node_pairwise(dest_cluster);
    if (pw != nullptr) {
      pairwise_from_json(*pw, entry);
    }
  }
}

json network_to_json_obj(const AnpNetwork& net) {
  json j;
  if (!net.name().empty()) {
    j["name"] = net.name();
  }
  if (!net.description().empty()) {
    j["description"] = net.description();
  }
  j["clusters"] = json::array();
  for (const AnpCluster* c : net.clusters()) {
    json cj;
    cj["name"] = c->name();
    cj["description"] = c->description();
    double x = 0, y = 0;
    if (net.cluster_position(c->name(), x, y)) {
      cj["x"] = x;
      cj["y"] = y;
    }
    cj["cluster_pairwise"] = pairwise_to_json(c->cluster_pairwise());
    cj["nodes"] = json::array();
    for (const AnpNode* n : c->nodes()) {
      json nj;
      nj["name"] = n->name();
      nj["description"] = n->description();
      nj["invert"] = n->invert();
      double nx = 0, ny = 0;
      if (net.node_position(n->name(), nx, ny)) {
        nj["x"] = nx;
        nj["y"] = ny;
      }
      json pri_map = json::object();
      for (const AnpCluster* dest : net.clusters()) {
        const NodePrioritizerSlot* slot = n->node_prioritizer(dest->name());
        if (slot != nullptr && !slot->empty()) {
          pri_map[dest->name()] = prioritizer_to_json(*slot);
        }
      }
      nj["node_prioritizers"] = pri_map;
      if (n->has_subnetwork()) {
        nj["subnetwork"] = network_to_json_obj(*n->subnetwork());
      }
      cj["nodes"].push_back(nj);
    }
    j["clusters"].push_back(cj);
  }
  if (net.alternatives_cluster() != nullptr) {
    j["alternatives_cluster"] = net.alternatives_cluster()->name();
  }
  const SynthesisOptions& syn = net.synthesis_options();
  j["synthesis"] = {
      {"kind", syn.kind == SynthesisKind::Additive         ? "additive"
               : syn.kind == SynthesisKind::Multiplicative ? "multiplicative"
                                                           : "custom"},
      {"custom_expr", syn.custom_expr},
  };

  const LimitMatrixOptions& lim = net.limit_matrix_options();
  const char* method = "calculus";
  switch (lim.method) {
    case LimitMatrixMethod::NewHierarchy:
      method = "newhierarchy";
      break;
    case LimitMatrixMethod::Sinks:
      method = "sinks";
      break;
    case LimitMatrixMethod::Calculus:
    default:
      method = "calculus";
      break;
  }
  j["limit_matrix"] = {
      {"method", method},
      {"error", lim.error},
      {"max_iters", lim.max_iters},
      {"use_hierarchy_formula", lim.use_hierarchy_formula},
      {"start_pow", lim.start_pow},
      {"with_limit", lim.with_limit},
      {"max_count", lim.max_count},
      {"straight_normalizer", lim.straight_normalizer},
  };
  return j;
}

void populate_network(AnpNetwork& net, const json& j) {
  if (j.contains("name")) {
    net.set_name(j.at("name").get<std::string>());
  }
  if (j.contains("description")) {
    net.set_description(j.at("description").get<std::string>());
  }
  // Pass 1: create clusters/nodes and layout metadata so names exist before
  // we wire connections and load pairwise matrices in pass 2.
  for (const auto& cj : j.at("clusters")) {
    const std::string cname = cj.at("name").get<std::string>();
    AnpCluster* existing = net.find_cluster(cname);
    AnpCluster& cluster =
        existing != nullptr ? *existing : net.add_cluster(cname);
    if (cj.contains("description")) {
      cluster.set_description(cj.at("description").get<std::string>());
    }
    if (cj.contains("x") && cj.contains("y")) {
      net.set_cluster_position(cname, cj.at("x").get<double>(),
                               cj.at("y").get<double>());
    }
    for (const auto& nj : cj.at("nodes")) {
      const std::string nname = nj.at("name").get<std::string>();
      if (net.find_node(nname) == nullptr) {
        net.add_node(cname, nname);
      }
      AnpNode& node = net.node(nname);
      if (nj.contains("description")) {
        node.set_description(nj.at("description").get<std::string>());
      }
      if (nj.contains("invert")) {
        node.set_invert(nj.at("invert").get<bool>());
      }
      if (nj.contains("x") && nj.contains("y")) {
        net.set_node_position(nname, nj.at("x").get<double>(),
                              nj.at("y").get<double>());
      }
    }
  }

  if (j.contains("alternatives_cluster")) {
    const std::string ac = j.at("alternatives_cluster").get<std::string>();
    if (net.find_cluster(ac) != nullptr) {
      net.set_alternatives_cluster(ac);
    }
  }

  // Pass 2: cluster/node prioritizers, implicit connections, and subnetworks.
  for (const auto& cj : j.at("clusters")) {
    AnpCluster& cluster = net.cluster(cj.at("name").get<std::string>());
    if (cj.contains("cluster_pairwise")) {
      pairwise_from_json(cluster.cluster_pairwise(), cj.at("cluster_pairwise"));
    }
    for (const auto& nj : cj.at("nodes")) {
      AnpNode& node = net.node(nj.at("name").get<std::string>());
      const json* pri = nullptr;
      if (nj.contains("node_prioritizers")) {
        pri = &nj.at("node_prioritizers");
      } else if (nj.contains("node_pairwise")) {
        // Legacy key: treat each entry as pairwise.
        pri = &nj.at("node_pairwise");
      }
      if (pri != nullptr) {
        for (auto it = pri->begin(); it != pri->end(); ++it) {
          load_prioritizer_entry(net, node, it.key(), it.value());
        }
      }
      if (nj.contains("subnetwork")) {
        AnpNetwork& sub = node.ensure_subnetwork();
        populate_network(sub, nj.at("subnetwork"));
      }
    }
  }

  if (j.contains("synthesis")) {
    SynthesisOptions syn;
    const std::string kind = j.at("synthesis").value("kind", "additive");
    if (kind == "multiplicative") {
      syn.kind = SynthesisKind::Multiplicative;
    } else if (kind == "custom") {
      syn.kind = SynthesisKind::Custom;
    } else {
      syn.kind = SynthesisKind::Additive;
    }
    syn.custom_expr = j.at("synthesis").value("custom_expr", "");
    net.set_synthesis_options(std::move(syn));
  }

  if (j.contains("limit_matrix")) {
    const json& lj = j.at("limit_matrix");
    LimitMatrixOptions lim;
    const std::string method = lj.value("method", "calculus");
    if (method == "newhierarchy" || method == "new_hierarchy" ||
        method == "hierarchy") {
      lim.method = LimitMatrixMethod::NewHierarchy;
    } else if (method == "sinks" || method == "sink") {
      lim.method = LimitMatrixMethod::Sinks;
    } else {
      lim.method = LimitMatrixMethod::Calculus;
    }
    lim.error = lj.value("error", lim.error);
    lim.max_iters = lj.value("max_iters", lim.max_iters);
    lim.use_hierarchy_formula =
        lj.value("use_hierarchy_formula", lim.use_hierarchy_formula);
    lim.start_pow = lj.value("start_pow", lim.start_pow);
    lim.with_limit = lj.value("with_limit", lim.with_limit);
    lim.max_count = lj.value("max_count", lim.max_count);
    lim.straight_normalizer =
        lj.value("straight_normalizer", lim.straight_normalizer);
    net.set_limit_matrix_options(std::move(lim));
  }
}

}  // namespace

std::string network_to_json(const AnpNetwork& network) {
  json doc;
  doc["format"] = "anpcpp";
  doc["version"] = 1;
  doc["network"] = network_to_json_obj(network);
  return doc.dump(2);
}

std::unique_ptr<AnpNetwork> network_from_json(const std::string& json_text) {
  json doc;
  try {
    doc = json::parse(json_text);
  } catch (const json::exception& e) {
    throw JsonIoError(std::string("JSON parse error: ") + e.what());
  }
  if (!doc.contains("network")) {
    throw JsonIoError("missing 'network' object");
  }
  if (doc.value("format", "") != "anpcpp") {
    throw JsonIoError("unsupported format (expected anpcpp)");
  }
  if (doc.value("version", 1) != 1) {
    throw JsonIoError("unsupported anpcpp JSON version");
  }

  // Skip auto-creating "Alternatives" when the file already defines that cluster.
  bool has_default_alts = false;
  for (const auto& cj : doc.at("network").at("clusters")) {
    if (cj.at("name").get<std::string>() ==
        AnpNetwork::kDefaultAlternativesCluster) {
      has_default_alts = true;
      break;
    }
  }
  auto net = std::make_unique<AnpNetwork>(has_default_alts);
  try {
    populate_network(*net, doc.at("network"));
  } catch (const json::exception& e) {
    throw JsonIoError(std::string("invalid network JSON: ") + e.what());
  } catch (const std::exception& e) {
    throw JsonIoError(e.what());
  }
  return net;
}

void save_network_file(const AnpNetwork& network, const std::string& path) {
  std::ofstream out(path);
  if (!out) {
    throw JsonIoError("cannot open file for write: " + path);
  }
  out << network_to_json(network);
}

std::unique_ptr<AnpNetwork> load_network_file(const std::string& path) {
  std::ifstream in(path);
  if (!in) {
    throw JsonIoError("cannot open file for read: " + path);
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return network_from_json(ss.str());
}

}  // namespace anpcpp
