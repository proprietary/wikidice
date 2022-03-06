type QueryCache = {
	[query: string]: string[];
};

class Autocomplete extends HTMLElement {
	private cache: QueryCache;
	private inputDelay: number = 300;
	private minQuery = 1;
	private maxOptions = 10;
	private api: string = '';
	private input: HTMLInputElement | null = null;
	private datalist: HTMLDataListElement | null = null;
	private datalistId: string;
	private debounce?: number;

	constructor() {
		super();
		this.cache = {};
		this.datalistId = 'so_libhack_wikidice__autocomplete_datalist_id';
	}

	static get observedAttributes() {
		return ['api', 'min-query', 'max-options'];
	}

	public attributeChangedCallback(name: string, oldValue: any, newValue: any) {
		if (oldValue === newValue) {
			return;
		}
		switch (name) {
		case 'api':
			this.api = newValue;
			break;
		case 'min-query':
			this.minQuery = newValue;
			break;
		case 'max-options':
			this.maxOptions = newValue;
			break;
		default:
			console.warn(`No attribute named "${name}" exists on <lh-autocomplete/>`);
		}
	}

	public connectedCallback() {
		const input = this.firstElementChild;
		if (input?.nodeName === 'INPUT') {
			this.input = input as HTMLInputElement;
		} else {
			throw new Error(`Child element passed to <lh-autocomplete/> must be an <input>`);
		}
		const dl = document.createElement('datalist');
		dl.id = this.datalistId;
		this.datalist = this.insertBefore(dl, this.input);
		this.input.setAttribute('autocomplete', 'off');
		this.input.setAttribute('list', this.datalistId);
		this.input.addEventListener('input', this.handleInput);
	}

	public disconnectedCallback() {
		this.input?.removeEventListener('input', this.handleInput);
		this.input?.removeAttribute('list');
		this.datalist?.remove();
	}

	private handleInput = (e: Event): any => {
		clearTimeout(this.debounce);
		this.debounce = setTimeout(async () => {
			const q = this.input?.value || '';
			let completions: string[] | undefined;
			if (Object.keys(this.cache).indexOf(q) !== -1) {
				completions = this.cache[q];
			} else {
				completions = await this.callApi(this.input?.value || '');
				completions = completions.map((x: string) => Autocomplete.dirtyCategoryName(x));
				this.cache[q] = completions;
			}
			this.updateDatalist(completions);
		}, this.inputDelay);
	}

	private async callApi(query: string): Promise<string[]> {
		if (query.length < this.minQuery) {
			return [];
		}
		const r = await fetch(`${this.api}${query}`);
		if (r.status !== 200) {
			console.error(`Failure to fetch autocomplete results: ${await r.text()}`);
			return [];
		}
		let completions = await r.json();
		return completions.slice(0, this.maxOptions);
	}

	private updateDatalist(completions: string[]) {
		while (this.datalist?.firstChild) {
			this.datalist.removeChild(this.datalist.firstChild);
		}
		for (const completion of completions) {
			const option = document.createElement('option');
			option.value = completion;
			this.datalist?.appendChild(option);
		}
	}

	// Make the category name human readable.
	private static dirtyCategoryName(category: string): string {
		let newName = '';
		for (let i = 0; i < category.length; ++i) {
			if (category[i] === '_') {
				newName += ' ';
			} else {
				newName += category[i];
			}
		}
		return newName;
	}
}

window.customElements.define('lh-autocomplete', Autocomplete);
