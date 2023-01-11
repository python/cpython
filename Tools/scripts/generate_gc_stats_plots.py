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
        item = vals["collection_time"] / vals["total_objects"].values
        if item.size == 0:
            item = np.array([0])
        gen_data.append(item)
    return gen_data

def get_gen_freq_per_us(df):
    gen_data = []
    for generation in range(3):
        vals = df[df["generation_number"] == generation]
        item = 1.0 / (vals["collection_time"] / vals["total_objects"]).values / 1.0e6
        if item.size == 0:
            item = np.array([0])
        gen_data.append(item)
    return gen_data

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

    names = ["First generation", "Second generation", "Third generation"]
    fig, (ax1, ax2, ax3, ax4) = plt.subplots(
        4, 1, figsize=(8, (2 + len(names) * 0.3) * 4), layout="constrained"
    )

    obj_percentage_data = get_obj_percentage_data(df)
    formatter = lambda val, pos: f"{val*100:.0f}%"
    violinplot_with_custom_formatting(
        ax1,
        obj_percentage_data,
        names,
        (0, len(names) + 1),
        "efficiency",
        "Generation efficiency stats",
        formatter=formatter,
    )

    formatter = lambda val, pos: f"{val*1000:.0f}ms"
    gen_time_data = get_gen_time_data(df)
    violinplot_with_custom_formatting(
        ax2,
        gen_time_data,
        names,
        (0, len(names) + 1),
        "time",
        "Generation Time stats",
        formatter=formatter,
    )

    formatter = lambda val, pos: f"{val*10.0e6:.0f}us"
    gen_time_data_per_obj = get_gen_time_data_per_obj(df)
    violinplot_with_custom_formatting(
        ax3,
        gen_time_data_per_obj,
        names,
        (0, len(names) + 1),
        "time",
        "Generation Time per obj stats",
        formatter=formatter,
    )

    formatter = lambda val, pos: f"{int(val)}obj/us"
    gen_freq_per_us = get_gen_freq_per_us(df)
    violinplot_with_custom_formatting(
        ax4,
        gen_freq_per_us,
        names,
        (0, len(names) + 1),
        "time",
        "Objects collected per us stats",
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


if __name__ == "__main__":
    main()
