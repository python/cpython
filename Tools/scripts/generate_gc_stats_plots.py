import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import argparse


def get_obj_percentage_data(df):
    gen_data = []
    for generation in range(3):
        vals = df[df["generation_number"] == generation]
        item = vals["collected_cycles"] / vals["total_objects"].values
        if item.size == 0:
            item = np.array([0])
        gen_data.append(item)
    return gen_data


def get_gen_time_data(df):
    gen_data = []
    for generation in range(3):
        vals = df[df["generation_number"] == generation]
        item = vals["collection_time"].values
        if item.size == 0:
            item = np.array([0])
        gen_data.append(item)
    return gen_data


def get_gen_time_data_per_obj(df):
    gen_data = []
    for generation in range(3):
        vals = df[df["generation_number"] == generation]
        item = vals["collection_time"] / (1 + vals["collected_cycles"]).values
        if item.size == 0:
            item = np.array([0])
        gen_data.append(item)
    return gen_data


def get_gen_freq_per_us(df):
    gen_data = []
    for generation in range(3):
        vals = df[df["generation_number"] == generation]
        item = (
            1.0
            / (vals["collection_time"] / (1 + vals["collected_cycles"])).values
            / 1.0e3
        )
        if item.size == 0:
            item = np.array([0])
        gen_data.append(item)
    return gen_data


def get_gc_summary(df):
    gen_totals = []
    for generation in range(3):
        vals = df[df["generation_number"] == generation]
        obj_collected = vals["collected_cycles"].sum()
        total_time = vals["collection_time"].sum()
        total_objs = vals["total_objects"].sum()
        gen_totals.append((obj_collected, total_time, total_objs))
    total_gc_time = df["collection_time"].sum()
    return gen_totals, total_gc_time


def gen_plot(df, output_filename):
    def violinplot_with_custom_formatting(
        ax, data, yticks, ylim, xlabel, title, formatter
    ):
        violin = ax.violinplot(
            data,
            vert=False,
            showmeans=True,
            showmedians=False,
            widths=1,
            quantiles=[[0.1, 0.9]] * len(data),
        )
        violin["cquantiles"].set_linestyle(":")
        ax.set_yticks(np.arange(len(yticks)) + 1, yticks)
        ax.set_ylim(ylim)
        ax.tick_params(axis="x", bottom=True, top=True, labelbottom=True, labeltop=True)
        ax.xaxis.set_major_formatter(formatter)
        ax.grid()
        ax.set_title(title)

    def barplot_with_custom_formatting(ax, data, title, ytitle, formatter):
        ax.bar(names, data)
        ax.set_ylabel(ytitle)
        ax.set_title(title)
        ax.yaxis.set_major_formatter(formatter)
        ax.grid()

    # names = ["First generation", "Second generation", "Third generation"]
    names = ["Gen-0", "Gen-1", "Gen-2"]
    # fig, (ax1, ax2, ax3, ax4) = plt.subplots(
    #     5, 1, figsize=(8, (2 + len(names) * 0.3) * 4), layout="constrained"
    # )
    fig = plt.figure(figsize=(10, 10), layout="constrained")

    ax1_0 = plt.subplot(7, 3, 1)
    ax1_1 = plt.subplot(7, 3, 2)
    ax1_2 = plt.subplot(7, 3, 3)
    ax2_2 = plt.subplot(7, 1, 3)
    ax2_3 = plt.subplot(7, 1, 4)
    ax2_4 = plt.subplot(7, 1, 5)
    ax2_5 = plt.subplot(7, 1, 6)

    figsize = (8, (2 + len(names) * 0.3) * 4)

    running_time_data = [x.sum() for x in get_gen_time_data(df)]
    formatter = lambda val, pos: f"{val*1000:.0f}"
    barplot_with_custom_formatting(
        ax1_0, running_time_data, "Total running time", "[ms]", formatter
    )

    object_number_data = [
        np.sum(df[df["generation_number"] == i]["collected_cycles"] / 1000.0)
        for i in range(3)
    ]
    formatter = lambda val, pos: f"{val:.0f}"
    barplot_with_custom_formatting(
        ax1_1, object_number_data, "Total object collected", "[x1000 obj]", formatter
    )

    object_per_time = np.divide(
        np.array(object_number_data), np.array(running_time_data)
    )
    formatter = lambda val, pos: f"{val:.0f}"
    barplot_with_custom_formatting(
        ax1_2, object_number_data, "Collected per run time", "[obj/ms]", formatter
    )

    obj_percentage_data = get_obj_percentage_data(df)
    formatter = lambda val, pos: f"{val*100:.0f}%"
    violinplot_with_custom_formatting(
        ax2_2,
        obj_percentage_data,
        names,
        (0, len(names) + 1),
        "efficiency",
        "Generation efficiency",
        formatter=formatter,
    )

    formatter = lambda val, pos: f"{val*1e3:.0f}ms"
    gen_time_data = get_gen_time_data(df)
    violinplot_with_custom_formatting(
        ax2_3,
        gen_time_data,
        names,
        (0, len(names) + 1),
        "time",
        "Generation Time",
        formatter=formatter,
    )

    formatter = lambda val, pos: f"{val*1e3:.0f}ms"
    gen_time_data_per_obj = get_gen_time_data_per_obj(df)
    violinplot_with_custom_formatting(
        ax2_4,
        gen_time_data_per_obj,
        names,
        (0, len(names) + 1),
        "time",
        "Time per obj collected",
        formatter=formatter,
    )

    formatter = lambda val, pos: f"{int(val)} obj/ms"
    gen_freq_per_us = get_gen_freq_per_us(df)
    violinplot_with_custom_formatting(
        ax2_5,
        gen_freq_per_us,
        names,
        (0, len(names) + 1),
        "time",
        "Objects collected per ms",
        formatter=formatter,
    )

    plt.savefig(output_filename)
    plt.close()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("file", help="The file to plot")
    parser.add_argument("output_filename", help="The output filename")
    args = parser.parse_args()

    df = pd.read_csv(args.file)
    gen_plot(df, args.output_filename)

    gen_totals, total_gc_time = get_gc_summary(df)
    for generation in range(3):
        gen_data = gen_totals[generation]
        print(
            f"Generation {generation}: {gen_data[0]} objects collected, {gen_data[1]:.2f} s, {gen_data[2]} total objects"
        )
    print("Total GC time: {:.2f} s".format(total_gc_time))


if __name__ == "__main__":
    main()
