/*
 * Copyright (C) 2011 Google Inc. All Rights Reserved.
 * Copyright (C) 2012 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TreeScope_h
#define TreeScope_h

#include "core/CoreExport.h"
#include "core/dom/TreeOrderedMap.h"
#include "core/html/forms/RadioButtonGroupScope.h"
#include "core/layout/HitTestRequest.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/text/AtomicString.h"

namespace blink {

class ContainerNode;
class DOMSelection;
class Document;
class Element;
class HTMLMapElement;
class HitTestResult;
class IdTargetObserverRegistry;
class SVGTreeScopeResources;
class ScopedStyleResolver;
class Node;

// A class which inherits both Node and TreeScope must call clearRareData() in
// its destructor so that the Node destructor no longer does problematic
// NodeList cache manipulation in the destructor.
class CORE_EXPORT TreeScope : public GarbageCollectedMixin {
 public:
  TreeScope* ParentTreeScope() const { return parent_tree_scope_; }

  TreeScope* OlderShadowRootOrParentTreeScope() const;
  bool IsInclusiveOlderSiblingShadowRootOrAncestorTreeScopeOf(
      const TreeScope&) const;

  Element* AdjustedFocusedElement() const;
  // Finds a retargeted element to the given argument, when the retargeted
  // element is in this TreeScope. Returns null otherwise.
  // TODO(kochi): once this algorithm is named in the spec, rename the method
  // name.
  Element* AdjustedElement(const Element&) const;
  Element* getElementById(const AtomicString&) const;
  const HeapVector<Member<Element>>& GetAllElementsById(
      const AtomicString&) const;
  bool HasElementWithId(const AtomicString& id) const;
  bool ContainsMultipleElementsWithId(const AtomicString& id) const;
  void AddElementById(const AtomicString& element_id, Element*);
  void RemoveElementById(const AtomicString& element_id, Element*);

  Document& GetDocument() const {
    DCHECK(document_);
    return *document_;
  }

  Node* AncestorInThisScope(Node*) const;

  void AddImageMap(HTMLMapElement*);
  void RemoveImageMap(HTMLMapElement*);
  HTMLMapElement* GetImageMap(const String& url) const;

  Element* ElementFromPoint(double x, double y) const;
  Element* HitTestPoint(double x, double y, const HitTestRequest&) const;
  HeapVector<Member<Element>> ElementsFromPoint(double x, double y) const;
  HeapVector<Member<Element>> ElementsFromHitTestResult(HitTestResult&) const;

  DOMSelection* GetSelection() const;

  Element* Retarget(const Element& target) const;

  Element* AdjustedFocusedElementInternal(const Element& target) const;

  // Find first anchor with the given name.
  // First searches for an element with the given ID, but if that fails, then
  // looks for an anchor with the given name. ID matching is always case
  // sensitive, but Anchor name matching is case sensitive in strict mode and
  // not case sensitive in quirks mode for historical compatibility reasons.
  Element* FindAnchor(const String& name);

  // Used by the basic DOM mutation methods (e.g., appendChild()).
  void AdoptIfNeeded(Node&);

  ContainerNode& RootNode() const { return *root_node_; }

  IdTargetObserverRegistry& GetIdTargetObserverRegistry() const {
    return *id_target_observer_registry_.Get();
  }

  RadioButtonGroupScope& GetRadioButtonGroupScope() {
    return radio_button_group_scope_;
  }

  bool IsInclusiveAncestorOf(const TreeScope&) const;
  unsigned short ComparePosition(const TreeScope&) const;

  const TreeScope* CommonAncestorTreeScope(const TreeScope& other) const;
  TreeScope* CommonAncestorTreeScope(TreeScope& other);

  Element* GetElementByAccessKey(const String& key) const;

  virtual void Trace(blink::Visitor*);

  ScopedStyleResolver* GetScopedStyleResolver() const {
    return scoped_style_resolver_.Get();
  }
  ScopedStyleResolver& EnsureScopedStyleResolver();
  void ClearScopedStyleResolver();

  SVGTreeScopeResources& EnsureSVGTreeScopedResources();

 protected:
  TreeScope(ContainerNode&, Document&);
  TreeScope(Document&);
  virtual ~TreeScope();

  void ResetTreeScope();
  void SetDocument(Document& document) { document_ = &document; }
  void SetParentTreeScope(TreeScope&);
  void SetNeedsStyleRecalcForViewportUnits();

 private:
  Element* HitTestPointInternal(Node*) const;

  Member<ContainerNode> root_node_;
  Member<Document> document_;
  Member<TreeScope> parent_tree_scope_;

  Member<TreeOrderedMap> elements_by_id_;
  Member<TreeOrderedMap> image_maps_by_name_;

  Member<IdTargetObserverRegistry> id_target_observer_registry_;

  Member<ScopedStyleResolver> scoped_style_resolver_;

  mutable Member<DOMSelection> selection_;

  RadioButtonGroupScope radio_button_group_scope_;

  Member<SVGTreeScopeResources> svg_tree_scoped_resources_;
};

inline bool TreeScope::HasElementWithId(const AtomicString& id) const {
  DCHECK(!id.IsNull());
  return elements_by_id_ && elements_by_id_->Contains(id);
}

inline bool TreeScope::ContainsMultipleElementsWithId(
    const AtomicString& id) const {
  return elements_by_id_ && elements_by_id_->ContainsMultiple(id);
}

DEFINE_COMPARISON_OPERATORS_WITH_REFERENCES(TreeScope)

HitTestResult HitTestInDocument(
    Document*,
    double x,
    double y,
    const HitTestRequest& = HitTestRequest::kReadOnly |
                            HitTestRequest::kActive);

}  // namespace blink

#endif  // TreeScope_h
