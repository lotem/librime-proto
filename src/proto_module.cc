#include <rime/component.h>
#include <rime/composition.h>
#include <rime/context.h>
#include <rime/menu.h>
#include <rime/schema.h>
#include <rime/service.h>
#include <rime_api.h>
#include <rime_proto.capnp.h>
#include "rime_proto_api.h"

using namespace rime;

void rime_commit_proto(RimeSessionId session_id, RIME_PROTO_BUILDER* commit_builder) {
    an<Session> session(Service::instance().GetSession(session_id));
    if (!session)
        return;
    const string& commit_text(session->commit_text());
    if (!commit_text.empty()) {
        auto* commit = (rime::proto::Commit::Builder*)commit_builder;
        commit->setText(commit_text);
        session->ResetCommitText();
    }
}

void rime_context_proto(RimeSessionId session_id, RIME_PROTO_BUILDER* context_builder) {
    an<Session> session = Service::instance().GetSession(session_id);
    if (!session)
        return;
    Context *ctx = session->context();
    if (!ctx)
        return;
    auto* context = (rime::proto::Context::Builder*)context_builder;
    context->setInput(ctx->input());
    context->setCaretPos(ctx->caret_pos());
    if (ctx->IsComposing()) {
        auto composition = context->getComposition();
        Preedit preedit = ctx->GetPreedit();
        composition.setLength(preedit.text.length());
        composition.setPreedit(preedit.text);
        composition.setCursorPos(preedit.caret_pos);
        composition.setSelStart(preedit.sel_start);
        composition.setSelEnd(preedit.sel_end);
        string commit_text = ctx->GetCommitText();
        if (!commit_text.empty()) {
            composition.setCommitTextPreview(commit_text);
        }
    }
    if (ctx->HasMenu()) {
        auto menu = context->getMenu();
        Segment &seg = ctx->composition().back();
        Schema *schema = session->schema();
        int page_size = schema ? schema->page_size() : 5;
        int selected_index = seg.selected_index;
        int page_number = selected_index / page_size;
        the<Page> page(seg.menu->CreatePage(page_size, page_number));
        if (page) {
            menu.setPageSize(page_size);
            menu.setPageNumber(page_number);
            menu.setIsLastPage(page->is_last_page);
            menu.setHighlightedCandidateIndex(selected_index % page_size);
            vector<string> labels;
            if (schema) {
                const string& select_keys = schema->select_keys();
                if (!select_keys.empty()) {
                    menu.setSelectKeys(select_keys);
                }
                Config* config = schema->config();
                auto src_labels = config->GetList("menu/alternative_select_labels");
                if (src_labels && (size_t)page_size <= src_labels->size()) {
                    auto dest_labels = menu.initSelectLabels(page_size);
                    for (size_t i = 0; i < (size_t)page_size; ++i) {
                        if (an<ConfigValue> value = src_labels->GetValueAt(i)) {
                            dest_labels.set(i, value->str());
                            labels.push_back(value->str());
                        }
                    }
                } else if (!select_keys.empty()) {
                    for (const char key : select_keys) {
                        labels.push_back(string(1, key));
                        if (labels.size() >= page_size)
                            break;
                    }
                }
            }
            int num_candidates = page->candidates.size();
            auto dest_candidates = menu.initCandidates(num_candidates);
            auto dest = dest_candidates.begin();
            int index = 0;
            for (const an<Candidate> &src : page->candidates) {
                dest->setText(src->text());
                string comment = src->comment();
                if (!comment.empty()) {
                    dest->setComment(comment);
                }
                string label = index < labels.size()
                    ? labels[index]
                    : std::to_string(index + 1);
                dest->setLabel(label);
                ++dest;
            }
        }
    }
}

void rime_status_proto(RimeSessionId session_id, RIME_PROTO_BUILDER* status_builder) {
    an<Session> session(Service::instance().GetSession(session_id));
    if (!session)
        return;
    Schema *schema = session->schema();
    Context *ctx = session->context();
    if (!schema || !ctx)
        return;
    auto* status = (rime::proto::Status::Builder*)status_builder;
    status->setSchemaId(schema->schema_id());
    status->setSchemaName(schema->schema_name());
    status->setIsDisabled(Service::instance().disabled());
    status->setIsComposing(ctx->IsComposing());
    status->setIsAsciiMode(ctx->get_option("ascii_mode"));
    status->setIsFullShape(ctx->get_option("full_shape"));
    status->setIsSimplified(ctx->get_option("simplification"));
    status->setIsTraditional(ctx->get_option("traditional"));
    status->setIsAsciiPunct(ctx->get_option("ascii_punct"));
}

static void rime_proto_initialize() {}

static void rime_proto_finalize() {}

static RimeCustomApi* rime_proto_get_api() {
    static RimeProtoApi s_api ={0};
    if (!s_api.data_size) {
        RIME_STRUCT_INIT(RimeProtoApi, s_api);
        s_api.commit_proto = &rime_commit_proto;
        s_api.context_proto = &rime_context_proto;
        s_api.status_proto = &rime_status_proto;
    }
    return (RimeCustomApi*)&s_api;
}

RIME_REGISTER_CUSTOM_MODULE(proto) {
    module->get_api = &rime_proto_get_api;
}
