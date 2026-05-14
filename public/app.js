const state = {
    records: [],
    queue: []
};

const viewTitles = {
    registerView: "환자 접수",
    queueView: "진료 대기 순서",
    recordsView: "환자 진료 기록",
    priorityView: "우선순위 안내"
};

const form = document.querySelector("#patientForm");
const recordTable = document.querySelector("#recordTable");
const queueTable = document.querySelector("#queueTable");
const searchInput = document.querySelector("#searchInput");
const refreshButton = document.querySelector("#refreshButton");
const completeButton = document.querySelector("#completeButton");
const resultBox = document.querySelector("#resultBox");
const completeBox = document.querySelector("#completeBox");
const statusBadge = document.querySelector("#serverStatus");
const viewTitle = document.querySelector("#viewTitle");

function priorityClass(priority) {
    if (priority <= 2) {
        return "high";
    }
    if (priority === 3) {
        return "mid";
    }
    return "";
}

function setStatus(text, className = "ok") {
    statusBadge.textContent = text;
    statusBadge.className = `status ${className}`;
}

function switchView(viewId) {
    document.querySelectorAll(".view").forEach((view) => {
        view.classList.toggle("active", view.id === viewId);
    });

    document.querySelectorAll(".nav-button").forEach((button) => {
        button.classList.toggle("active", button.dataset.view === viewId);
    });

    viewTitle.textContent = viewTitles[viewId];
}

function renderSummary() {
    document.querySelector("#queueCount").textContent = state.queue.length;
    document.querySelector("#urgentQueueCount").textContent = state.queue.filter((patient) => patient.finalPriority <= 2).length;
    document.querySelector("#totalCount").textContent = state.records.length;
}

function renderQueue() {
    queueTable.innerHTML = "";
    completeButton.disabled = state.queue.length === 0;

    if (state.queue.length === 0) {
        const row = document.createElement("tr");
        row.innerHTML = `<td colspan="6">현재 진료 대기 환자가 없습니다.</td>`;
        queueTable.appendChild(row);
        return;
    }

    state.queue.forEach((patient, index) => {
        const row = document.createElement("tr");
        row.className = index === 0 ? "next-row" : "";
        row.innerHTML = `
            <td>${index + 1}</td>
            <td><span class="priority-badge ${priorityClass(patient.finalPriority)}">${patient.finalPriority}단계</span></td>
            <td>${patient.name}</td>
            <td>${patient.symptom}</td>
            <td>${patient.patientType}</td>
            <td>${patient.time}</td>
        `;
        queueTable.appendChild(row);
    });
}

function renderRecords() {
    const keyword = searchInput.value.trim().toLowerCase();
    const filtered = state.records
        .filter((record) => {
            const text = `${record.name} ${record.symptom}`.toLowerCase();
            return text.includes(keyword);
        })
        .slice()
        .reverse();

    recordTable.innerHTML = "";

    if (filtered.length === 0) {
        const row = document.createElement("tr");
        row.innerHTML = `<td colspan="4">표시할 기록이 없습니다.</td>`;
        recordTable.appendChild(row);
        return;
    }

    for (const record of filtered) {
        const row = document.createElement("tr");
        row.innerHTML = `
            <td>${record.time}</td>
            <td>${record.name}</td>
            <td>${record.symptom}</td>
            <td><span class="priority-badge ${priorityClass(record.priority)}">${record.priority}단계</span></td>
        `;
        recordTable.appendChild(row);
    }
}

async function loadRecords() {
    const response = await fetch("/api/records");
    if (!response.ok) {
        throw new Error("records request failed");
    }
    const data = await response.json();
    state.records = data.records || [];
}

async function loadQueue() {
    const response = await fetch("/api/queue");
    if (!response.ok) {
        throw new Error("queue request failed");
    }
    const data = await response.json();
    state.queue = data.queue || [];
}

async function refreshAll() {
    try {
        await Promise.all([loadRecords(), loadQueue()]);
        renderSummary();
        renderQueue();
        renderRecords();
        setStatus("서버 연결됨", "ok");
    } catch (error) {
        setStatus("서버 연결 실패", "error");
    }
}

function showRegisterResult(data) {
    resultBox.hidden = false;
    resultBox.innerHTML = `
        <strong>${data.name}</strong> 환자가 ${data.patientType}로 접수되었습니다.<br>
        대기번호 ${data.patientNo}, 현재 증상 중요도 ${data.currentPriority}단계, 최종 중요도 ${data.finalPriority}단계<br>
        최근 기록: ${data.recentRecord || "없음"}
    `;
}

function showCompleteResult(data) {
    completeBox.hidden = false;

    if (!data.completed) {
        completeBox.textContent = "진료 완료 처리할 대기 환자가 없습니다.";
        return;
    }

    completeBox.innerHTML = `
        <strong>${data.name}</strong> 환자의 진료를 완료했습니다.
        (${data.symptom}, 중요도 ${data.finalPriority}단계)
    `;
}

document.querySelectorAll(".nav-button").forEach((button) => {
    button.addEventListener("click", () => switchView(button.dataset.view));
});

form.addEventListener("submit", async (event) => {
    event.preventDefault();

    const formData = new URLSearchParams(new FormData(form));

    const response = await fetch("/api/register", {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded"
        },
        body: formData
    });

    if (!response.ok) {
        resultBox.hidden = false;
        resultBox.textContent = "접수 저장에 실패했습니다.";
        return;
    }

    const data = await response.json();
    showRegisterResult(data);
    form.reset();
    await refreshAll();
    switchView("queueView");
});

completeButton.addEventListener("click", async () => {
    const response = await fetch("/api/complete", {
        method: "POST"
    });

    if (!response.ok) {
        completeBox.hidden = false;
        completeBox.textContent = "진료 완료 처리에 실패했습니다.";
        return;
    }

    const data = await response.json();
    showCompleteResult(data);
    await refreshAll();
});

searchInput.addEventListener("input", renderRecords);
refreshButton.addEventListener("click", refreshAll);

refreshAll();
