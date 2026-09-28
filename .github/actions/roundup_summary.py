import json
import os
from datetime import date, timedelta
from typing import Iterable

import requests

ISSUE_ENDPOINT = "https://github.com/python/cpython/issues"
SEARCH_ENDPOINT = "https://api.github.com/search/issues"
GRAPHQL_ENDPOINT = "https://api.github.com/graphql"
MAILGUN_ENDPOINT = "https://api.mailgun.net/v3/mg.python.org"


def get_issue_counts(token: str) -> tuple[int, int]:
    """use the GraphQL API to get the number of opened and closed issues
    without having to query every single issue and count them."""
    data = {
        "query": """
        {
            repository(owner: "python", name: "cpython") {
                open: issues(states: OPEN) { totalCount }
                closed: issues(states: CLOSED) { totalCount }
            }
        }
        """
    }
    response = requests.post(GRAPHQL_ENDPOINT, json=data, headers={
        "Authorization": f"Bearer {token}",
        "accept": "application/vnd.github.v3+json"
    })
    repo = response.json()["data"]["repository"]
    open = repo["open"]["totalCount"]
    closed = repo["closed"]["totalCount"]
    return open, closed

def get_issues(filters: Iterable[str], token: str, all_: bool = True):
    """return a list of results from the Github search API"""
    # TODO: if there are more than 100 issues, we need to include pagination
    # this doesn't occur very often, but it should still be included just incase.
    data = {"query": f"""
        {{
            search(query:"{' '.join(filters)}" type: ISSUE first: 100)
            {{
                pageInfo {{ hasNextPage endCursor startCursor }}
                nodes {{
                    ... on Issue {{
                        title number author {{ login }} closedAt createdAt
                    }}
                }}
            }}
        }}
    """}
    response = requests.post(GRAPHQL_ENDPOINT, json=data, headers={
        "Authorization": f"Bearer {token}",
        "accept": "application/vnd.github.v3+json"
    })
    return response.json()["data"]["search"]["nodes"]

def send_report(payload: str, token: str) -> int:
    """send the report using the Mailgun API"""
    params = {
        "from": "Cpython Issues <github@mg.python.org>",
        "to": "",
        "subject": "Summary of Python tracker Issues",
        "template": "issue-tracker-template",
        "o:tracking-clicks": "no",
        "h:X-Mailgun-Variables": payload,
    }
    response = requests.post(
        MAILGUN_ENDPOINT,
        auth=("api", token),
        json=params)
    return response.status_code

if __name__ == '__main__':
    date_from = date.today() - timedelta(days=7)
    github_token = os.environ.get("github_api_token")
    mailgun_token = os.environ.get("mailgun_api_key")

    total_open, total_closed = get_issue_counts(github_token)
    closed = get_issues(("repo:python/cpython", f"closed:>{date_from}", "type:issue"), github_token)
    opened = get_issues(("repo:python/cpython", "state:open", f"created:>{date_from}", "type:issue"), github_token)
    most_discussed = get_issues(
        ("repo:python/cpython", "state:open", "type:issue", "sort:comments"),
        github_token,
        False)
    no_comments = get_issues(
        ("repo:python/cpython", "state:open", "type:issue", "comments:0", "sort:updated"),
        github_token,
        False)

    payload = {
        "opened_issues": opened,
        "closed_issues": closed,
        "most_discussed": most_discussed[:10],
        "no_comments": no_comments[:15],
        "total_open": total_open,
        "total_closed": total_closed,
        "week_delta": len(opened) - len(closed),
    }
    status_code = send_report(json.dumps(payload), mailgun_token)
    if status_code == 200:
        print("successfully sent email")
    else:
        # in this case, fail the GitHub action
        print("failed to send email", status_code)
        exit(1)
