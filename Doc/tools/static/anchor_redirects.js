const script = document.getElementById("python-docs-anchor-redirects");
const redirects = JSON.parse(script.textContent);

function redirectAnchor() {
    const anchor = window.location.hash.slice(1);
    if (!anchor) {
        return;
    }

    if (document.getElementById(anchor)) {
        return;
    }

    const target = redirects[anchor];
    if (!target) {
        return;
    }
    const targetUrl = new URL(target, window.location.href).href;
    if (targetUrl !== window.location.href) {
        window.location.replace(targetUrl);
    }
}

window.addEventListener("hashchange", redirectAnchor);
redirectAnchor();
