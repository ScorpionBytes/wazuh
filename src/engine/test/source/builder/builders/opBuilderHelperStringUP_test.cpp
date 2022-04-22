/* Copyright (C) 2015-2022, Wazuh Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include <gtest/gtest.h>
#include "testUtils.hpp"
#include <vector>

#include "opBuilderHelperMap.hpp"

using namespace builder::internals::builders;

using FakeTrFn = std::function<void(std::string)>;
static FakeTrFn tr = [](std::string msg){};

auto createEvent = [](const char * json){
    return std::make_shared<Base::EventHandler>(std::make_shared<json::Document>(json));
};

// Build ok
TEST(opBuilderHelperStringUP, Builds)
{
    Document doc{R"({
        "normalize":
            {"field2normalize": "+s_up/abcd"}
    })"};
    ASSERT_NO_THROW(opBuilderHelperStringUP(doc.get("/normalize"), tr));
}

// Build incorrect number of arguments
TEST(opBuilderHelperStringUP, Builds_incorrect_number_of_arguments)
{
    Document doc{R"({
        "normalize":
            {"field2normalize": "+s_up/test_value/test_value2"}
    })"};
    ASSERT_THROW(opBuilderHelperStringUP(doc.get("/normalize"), tr), std::runtime_error);
}

// Test ok: static values
TEST(opBuilderHelperStringUP, Static_string_ok)
{
    Document doc{R"({
        "normalize":
            {"fieltToCreate": "+s_up/asd123asd"}
    })"};

    Observable input = observable<>::create<Event>(
        [=](auto s)
        {
            s.on_next(createEvent(R"(
                {"not_fieltToCreate": "qwe"}
            )"));
            s.on_next(createEvent(R"(
                {"not_fieltToCreate": "ASD123asd"}
            )"));
            s.on_next(createEvent(R"(
                {"not_fieltToCreate": "ASD"}
            )"));
            s.on_completed();
        });

    Lifter lift = opBuilderHelperStringUP(doc.get("/normalize"), tr);
    Observable output = lift(input);
    vector<Event> expected;
    output.subscribe([&](Event e) { expected.push_back(e); });
    ASSERT_EQ(expected.size(), 3);
    ASSERT_STREQ(expected[0]->getEvent()->get("/fieltToCreate").GetString(), "ASD123ASD");
    ASSERT_STREQ(expected[1]->getEvent()->get("/fieltToCreate").GetString(), "ASD123ASD");
    ASSERT_STREQ(expected[2]->getEvent()->get("/fieltToCreate").GetString(), "ASD123ASD");
}

// Test ok: dynamic values (string)
TEST(opBuilderHelperStringUP, Dynamics_string_ok)
{

    Document doc{R"({
        "normalize":
            {"fieltToCreate": "+s_up/$srcField"}
    })"};

    Observable input = observable<>::create<Event>(
        [=](auto s)
        {
            s.on_next(createEvent(R"(
                {"srcField": "qwe"}
            )"));
            s.on_next(createEvent(R"(
                {"srcField": "ASD123asd"}
            )"));
            s.on_next(createEvent(R"(
                {"srcField": "ASD"}
            )"));
            s.on_completed();
        });

    Lifter lift = opBuilderHelperStringUP(doc.get("/normalize"), tr);
    Observable output = lift(input);
    vector<Event> expected;
    output.subscribe([&](Event e) { expected.push_back(e); });
    ASSERT_EQ(expected.size(), 3);
    ASSERT_STREQ(expected[0]->getEvent()->get("/fieltToCreate").GetString(), "QWE");
    ASSERT_STREQ(expected[1]->getEvent()->get("/fieltToCreate").GetString(), "ASD123ASD");
    ASSERT_STREQ(expected[2]->getEvent()->get("/fieltToCreate").GetString(), "ASD");
}

TEST(opBuilderHelperStringUP, Multilevel_src)
{
    Document doc{R"({
        "normalize":
            {"fieltToCreate": "+s_lo/$a.b.c.srcField"}
    })"};

    Observable input = observable<>::create<Event>(
        [=](auto s)
        {
            s.on_next(createEvent(R"(
                {"a": {"b": {"c": {"srcField": "qwe"}}}}
            )"));
            s.on_next(createEvent(R"(
                {"a": {"b": {"c": {"srcField": "ASD123asd"}}}}
            )"));
            s.on_next(createEvent(R"(
                {"a": {"b": {"c": {"srcField": "ASD"}}}}
            )"));
            s.on_completed();
        });

    Lifter lift = opBuilderHelperStringLO(doc.get("/normalize"), tr);
    Observable output = lift(input);
    vector<Event> expected;
    output.subscribe([&](Event e) { expected.push_back(e); });
    ASSERT_EQ(expected.size(), 3);
    ASSERT_STREQ(expected[0]->getEvent()->get("/fieltToCreate").GetString(), "qwe");
    ASSERT_STREQ(expected[1]->getEvent()->get("/fieltToCreate").GetString(), "asd123asd");
    ASSERT_STREQ(expected[2]->getEvent()->get("/fieltToCreate").GetString(), "asd");
}

TEST(opBuilderHelperStringUP, Multilevel_dst)
{
    Document doc{R"({
        "normalize":
            {"a.b.fieltToCreate.2": "+s_lo/$a.b.c.srcField"}
    })"};

    Observable input = observable<>::create<Event>(
        [=](auto s)
        {
            s.on_next(createEvent(R"(
                {"a": {"b": {"c": {"srcField": "qwe"}}}}
            )"));
            s.on_next(createEvent(R"(
                {"a": {"b": {"c": {"srcField": "ASD123asd"}}}}
            )"));
            s.on_next(createEvent(R"(
                {"a": {"b": {"c": {"srcField": "ASD"}}}}
            )"));
            s.on_completed();
        });

    Lifter lift = opBuilderHelperStringUP(doc.get("/normalize"), tr);
    Observable output = lift(input);
    vector<Event> expected;
    output.subscribe([&](Event e) { expected.push_back(e); });
    ASSERT_EQ(expected.size(), 3);
    ASSERT_STREQ(expected[0]->getEvent()->get("/a/b/fieltToCreate/2").GetString(), "QWE");
    ASSERT_STREQ(expected[1]->getEvent()->get("/a/b/fieltToCreate/2").GetString(), "ASD123ASD");
    ASSERT_STREQ(expected[2]->getEvent()->get("/a/b/fieltToCreate/2").GetString(), "ASD");
}

TEST(opBuilderHelperStringUP, Exist_dst)
{
    Document doc{R"({
        "normalize":
            {"a.b": "+s_lo/$a.b.c.srcField"}
    })"};

    Observable input = observable<>::create<Event>(
        [=](auto s)
        {
            s.on_next(createEvent(R"(
                {"a": {"b": {"c": {"srcField": "qwe"}}}}
            )"));
            s.on_next(createEvent(R"(
                {"a": {"b": {"c": {"srcField": "ASD123asd"}}}}
            )"));
            s.on_next(createEvent(R"(
                {"a": {"b": {"c": {"srcField": "ASD"}}}}
            )"));
            s.on_completed();
        });

    Lifter lift = opBuilderHelperStringUP(doc.get("/normalize"), tr);
    Observable output = lift(input);
    vector<Event> expected;
    output.subscribe([&](Event e) { expected.push_back(e); });
    ASSERT_EQ(expected.size(), 3);
    ASSERT_STREQ(expected[0]->getEvent()->get("/a/b").GetString(), "QWE");
    ASSERT_STREQ(expected[1]->getEvent()->get("/a/b").GetString(), "ASD123ASD");
    ASSERT_STREQ(expected[2]->getEvent()->get("/a/b").GetString(), "ASD");
}

TEST(opBuilderHelperStringUP, Not_exist_src)
{
    Document doc{R"({
    "normalize":
            {"a.b": "+s_lo/$srcField"}
    })"};

    Observable input = observable<>::create<Event>(
        [=](auto s)
        {
            s.on_next(createEvent(R"(
                {"a": {"b": "QWE"}}
            )"));
            s.on_next(createEvent(R"(
                {"c": {"d": "QWE123"}}
            )"));
            s.on_completed();
        });

    Lifter lift = opBuilderHelperStringUP(doc.get("/normalize"), tr);
    Observable output = lift(input);
    vector<Event> expected;
    output.subscribe([&](Event e) { expected.push_back(e); });
    ASSERT_EQ(expected.size(), 2);
    ASSERT_STREQ(expected[0]->getEvent()->get("/a/b").GetString(), "QWE");
    ASSERT_FALSE(expected[1]->getEvent()->exists("/a/b"));
}

TEST(opBuilderHelperStringUP, Src_not_string)
{
    Document doc{R"({
        "normalize":
            {"fieltToCreate": "+s_lo/$srcField123"}
    })"};

    Observable input = observable<>::create<Event>(
        [=](auto s)
        {
            s.on_next(createEvent(R"(
                {"srcField": "qwe"}
            )"));
            s.on_next(createEvent(R"(
                {"srcField": "ASD123asd"}
            )"));
            s.on_next(createEvent(R"(
                {"srcField": "ASD"}
            )"));
            s.on_completed();
        });

    Lifter lift = opBuilderHelperStringUP(doc.get("/normalize"), tr);
    Observable output = lift(input);
    vector<Event> expected;
    output.subscribe([&](Event e) { expected.push_back(e); });
    ASSERT_EQ(expected.size(), 3);
    ASSERT_FALSE(expected[0]->getEvent()->exists("/fieltToCreate"));
    ASSERT_FALSE(expected[1]->getEvent()->exists("/fieltToCreate"));
    ASSERT_FALSE(expected[2]->getEvent()->exists("/fieltToCreate"));
}