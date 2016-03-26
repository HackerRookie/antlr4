﻿/*
 * [The "BSD license"]
 *  Copyright (c) 2016 Mike Lischke
 *  Copyright (c) 2013 Terence Parr
 *  Copyright (c) 2013 Dan McLaughlin
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 *  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 *  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "BitSet.h"
#include "ATNConfig.h"
#include "ConfigLookup.h"

namespace org {
namespace antlr {
namespace v4 {
namespace runtime {
namespace atn {

  // Simpler hasher and comparer variants than those in ATNConfig (less fields, no murmur hash).
  struct SimpleATNConfigHasher
  {
    size_t operator()(const ATNConfig &k) const;
  };

  struct SimpleATNConfigComparer {
    bool operator()(const ATNConfig &lhs, const ATNConfig &rhs) const;
  };
  
  /// Specialized set that can track info about the set, with support for combining similar configurations using a
  /// graph-structured stack.
  class ATNConfigSet {
  public:
    /// <summary>
    /// All configs but hashed by (s, i, _, pi) not including context. Wiped out
    /// when we go readonly as this set becomes a DFA state.
    /// </summary>
    ConfigLookup *configLookup;

    /// <summary>
    /// Track the elements as they are added to the set; supports get(i) </summary>
    std::vector<ATNConfig *> configs;

    // TODO: these fields make me pretty uncomfortable but nice to pack up info together, saves recomputation
    // TODO: can we track conflicts as they are added to save scanning configs later?
    int uniqueAlt;

    antlrcpp::BitSet *conflictingAlts;

    // Used in parser and lexer. In lexer, it indicates we hit a pred
    // while computing a closure operation.  Don't make a DFA state from this.
    bool hasSemanticContext;
    bool dipsIntoOuterContext;

    /// <summary>
    /// Indicates that this configuration set is part of a full context
    ///  LL prediction. It will be used to determine how to merge $. With SLL
    ///  it's a wildcard whereas it is not for LL context merge.
    /// </summary>
    const bool fullCtx;

    ATNConfigSet(bool fullCtx = true, ConfigLookup *lookup = nullptr);
    ATNConfigSet(ATNConfigSet *old);

    virtual ~ATNConfigSet();

    virtual bool add(ATNConfig *config);

    /// <summary>
    /// Adding a new config means merging contexts with existing configs for
    /// {@code (s, i, pi, _)}, where {@code s} is the
    /// <seealso cref="ATNConfig#state"/>, {@code i} is the <seealso cref="ATNConfig#alt"/>, and
    /// {@code pi} is the <seealso cref="ATNConfig#semanticContext"/>. We use
    /// {@code (s,i,pi)} as key.
    /// <p/>
    /// This method updates <seealso cref="#dipsIntoOuterContext"/> and
    /// <seealso cref="#hasSemanticContext"/> when necessary.
    /// </summary>
    virtual bool add(ATNConfig *config, misc::DoubleKeyMap<PredictionContext*, PredictionContext*, PredictionContext*> *mergeCache);

    /// <summary>
    /// Return a List holding list of configs </summary>
    virtual std::vector<ATNConfig*> elements();

    virtual std::vector<ATNState*> *getStates();

    virtual std::vector<SemanticContext*> getPredicates();

    virtual ATNConfig *get(int i);

    virtual void optimizeConfigs(ATNSimulator *interpreter);

    bool addAll(ATNConfigSet *other);

    virtual bool equals(ATNConfigSet *other);
    virtual int hashCode();
    virtual size_t size();
    virtual bool isEmpty();
    virtual bool contains(ATNConfig *o);
    virtual void clear();
    virtual bool isReadonly();
    virtual void setReadonly(bool readonly);
    virtual std::wstring toString();
    virtual bool remove(void *o);

  protected:
    /// Indicates that the set of configurations is read-only. Do not
    ///  allow any code to manipulate the set; DFA states will point at
    ///  the sets and they must not change. This does not protect the other
    ///  fields; in particular, conflictingAlts is set after
    ///  we've made this readonly.
    bool _readonly;

  private:
    int _cachedHashCode;

    void InitializeInstanceFields();
  };

} // namespace atn
} // namespace runtime
} // namespace v4
} // namespace antlr
} // namespace org

namespace std {
  template <> struct hash<std::vector<ATNConfig *>>
  {
    size_t operator() (const std::vector<ATNConfig *> &vector) const
    {
      std::size_t seed = 0;
      for (auto &config : vector) {
        seed ^= config->hashCode() + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      }
      return seed;
    }
  };
}
